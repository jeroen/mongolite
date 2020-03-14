/*
 * Copyright 2014 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mongoc-config.h"

#include "mongoc-handshake.h"
#include "mongoc-handshake-private.h"

#include "mongoc-error.h"
#include "mongoc-host-list-private.h"
#include "mongoc-log.h"
#include "mongoc-topology-private.h"
#include "mongoc-topology-description-apm-private.h"
#include "mongoc-client-private.h"
#include "mongoc-cmd-private.h"
#include "mongoc-uri-private.h"
#include "mongoc-util-private.h"
#include "mongoc-trace-private.h"

#include "utlist.h"

static bool
_mongoc_topology_reconcile_add_nodes (mongoc_server_description_t *sd,
                                      mongoc_topology_t *topology)
{
   mongoc_topology_scanner_t *scanner = topology->scanner;

   /* quickly search by id, then check if a node for this host was retired in
    * this scan. */
   if (!mongoc_topology_scanner_get_node (scanner, sd->id) &&
       !mongoc_topology_scanner_has_node_for_host (scanner, &sd->host)) {
      mongoc_topology_scanner_add (scanner, &sd->host, sd->id);
      mongoc_topology_scanner_scan (scanner, sd->id);
   }

   return true;
}

void
mongoc_topology_reconcile (mongoc_topology_t *topology)
{
   mongoc_topology_description_t *description;
   mongoc_set_t *servers;
   mongoc_server_description_t *sd;
   int i;
   mongoc_topology_scanner_node_t *ele, *tmp;

   description = &topology->description;
   servers = description->servers;

   /* Add newly discovered nodes */
   for (i = 0; i < (int) servers->items_len; i++) {
      sd = (mongoc_server_description_t *) mongoc_set_get_item (servers, i);
      _mongoc_topology_reconcile_add_nodes (sd, topology);
   }

   /* Remove removed nodes */
   DL_FOREACH_SAFE (topology->scanner->nodes, ele, tmp)
   {
      if (!mongoc_topology_description_server_by_id (
             description, ele->id, NULL)) {
         mongoc_topology_scanner_node_retire (ele);
      }
   }
}


/* call this while already holding the lock */
static bool
_mongoc_topology_update_no_lock (uint32_t id,
                                 const bson_t *ismaster_response,
                                 int64_t rtt_msec,
                                 mongoc_topology_t *topology,
                                 const bson_error_t *error /* IN */)
{
   mongoc_topology_description_handle_ismaster (
      &topology->description, id, ismaster_response, rtt_msec, error);

   /* return false if server removed from topology */
   return mongoc_topology_description_server_by_id (
             &topology->description, id, NULL) != NULL;
}


/*
 *-------------------------------------------------------------------------
 *
 * _mongoc_topology_scanner_setup_err_cb --
 *
 *       Callback method to handle errors during topology scanner node
 *       setup, typically DNS or SSL errors.
 *
 *-------------------------------------------------------------------------
 */

void
_mongoc_topology_scanner_setup_err_cb (uint32_t id,
                                       void *data,
                                       const bson_error_t *error /* IN */)
{
   mongoc_topology_t *topology;

   BSON_ASSERT (data);

   topology = (mongoc_topology_t *) data;

   mongoc_topology_description_handle_ismaster (&topology->description,
                                                id,
                                                NULL /* ismaster reply */,
                                                -1 /* rtt_msec */,
                                                error);
}


/*
 *-------------------------------------------------------------------------
 *
 * _mongoc_topology_scanner_cb --
 *
 *       Callback method to handle ismaster responses received by async
 *       command objects.
 *
 *       NOTE: This method locks the given topology's mutex.
 *
 *-------------------------------------------------------------------------
 */

void
_mongoc_topology_scanner_cb (uint32_t id,
                             const bson_t *ismaster_response,
                             int64_t rtt_msec,
                             void *data,
                             const bson_error_t *error /* IN */)
{
   mongoc_topology_t *topology;
   mongoc_server_description_t *sd;

   BSON_ASSERT (data);

   topology = (mongoc_topology_t *) data;

   bson_mutex_lock (&topology->mutex);
   sd = mongoc_topology_description_server_by_id (
      &topology->description, id, NULL);

   /* Server Discovery and Monitoring Spec: "Once a server is connected, the
    * client MUST change its type to Unknown only after it has retried the
    * server once." */
   if (!ismaster_response && sd && sd->type != MONGOC_SERVER_UNKNOWN) {
      _mongoc_topology_update_no_lock (
         id, ismaster_response, rtt_msec, topology, error);

      /* add another ismaster call to the current scan - the scan continues
       * until all commands are done */
      mongoc_topology_scanner_scan (topology->scanner, sd->id);
   } else {
      _mongoc_topology_update_no_lock (
         id, ismaster_response, rtt_msec, topology, error);

      /* The processing of the ismaster results above may have added/removed
       * server descriptions. We need to reconcile that with our monitoring
       * agents
       */
      mongoc_topology_reconcile (topology);

      mongoc_cond_broadcast (&topology->cond_client);
   }

   bson_mutex_unlock (&topology->mutex);
}

/*
 *-------------------------------------------------------------------------
 *
 * mongoc_topology_new --
 *
 *       Creates and returns a new topology object.
 *
 * Returns:
 *       A new topology object.
 *
 * Side effects:
 *       None.
 *
 *-------------------------------------------------------------------------
 */
mongoc_topology_t *
mongoc_topology_new (const mongoc_uri_t *uri, bool single_threaded)
{
   int64_t heartbeat_default;
   int64_t heartbeat;
   mongoc_topology_t *topology;
   bool topology_valid;
   mongoc_topology_description_type_t init_type;
   const char *service;
   char *prefixed_service;
   uint32_t id;
   const mongoc_host_list_t *hl;
   mongoc_rr_data_t rr_data;

   BSON_ASSERT (uri);

#ifndef MONGOC_ENABLE_CRYPTO
   if (mongoc_uri_get_option_as_bool (
          uri, MONGOC_URI_RETRYWRITES, MONGOC_DEFAULT_RETRYWRITES)) {
      /* retryWrites requires sessions, which require crypto - just warn */
      MONGOC_WARNING (
         "retryWrites not supported without an SSL crypto library");
   }
#endif

   topology = (mongoc_topology_t *) bson_malloc0 (sizeof *topology);
   topology->session_pool = NULL;
   heartbeat_default =
      single_threaded ? MONGOC_TOPOLOGY_HEARTBEAT_FREQUENCY_MS_SINGLE_THREADED
                      : MONGOC_TOPOLOGY_HEARTBEAT_FREQUENCY_MS_MULTI_THREADED;

   heartbeat = mongoc_uri_get_option_as_int32 (
      uri, MONGOC_URI_HEARTBEATFREQUENCYMS, heartbeat_default);

   mongoc_topology_description_init (&topology->description, heartbeat);

   topology->description.set_name =
      bson_strdup (mongoc_uri_get_replica_set (uri));

   topology->uri = mongoc_uri_copy (uri);

   topology->single_threaded = single_threaded;
   if (single_threaded) {
      /* Server Selection Spec:
       *
       *   "Single-threaded drivers MUST provide a "serverSelectionTryOnce"
       *   mode, in which the driver scans the topology exactly once after
       *   server selection fails, then either selects a server or raises an
       *   error.
       *
       *   "The serverSelectionTryOnce option MUST be true by default."
       */
      topology->server_selection_try_once = mongoc_uri_get_option_as_bool (
         uri, MONGOC_URI_SERVERSELECTIONTRYONCE, true);
   } else {
      topology->server_selection_try_once = false;
   }

   topology->server_selection_timeout_msec = mongoc_uri_get_option_as_int32 (
      topology->uri,
      MONGOC_URI_SERVERSELECTIONTIMEOUTMS,
      MONGOC_TOPOLOGY_SERVER_SELECTION_TIMEOUT_MS);

   /* tests can override this */
   topology->min_heartbeat_frequency_msec =
      MONGOC_TOPOLOGY_MIN_HEARTBEAT_FREQUENCY_MS;

   topology->local_threshold_msec =
      mongoc_uri_get_local_threshold_option (topology->uri);

   /* Total time allowed to check a server is connectTimeoutMS.
    * Server Discovery And Monitoring Spec:
    *
    *   "The socket used to check a server MUST use the same connectTimeoutMS as
    *   regular sockets. Multi-threaded clients SHOULD set monitoring sockets'
    *   socketTimeoutMS to the connectTimeoutMS."
    */
   topology->connect_timeout_msec =
      mongoc_uri_get_option_as_int32 (topology->uri,
                                      MONGOC_URI_CONNECTTIMEOUTMS,
                                      MONGOC_DEFAULT_CONNECTTIMEOUTMS);

   topology->scanner_state = MONGOC_TOPOLOGY_SCANNER_OFF;
   topology->scanner =
      mongoc_topology_scanner_new (topology->uri,
                                   _mongoc_topology_scanner_setup_err_cb,
                                   _mongoc_topology_scanner_cb,
                                   topology,
                                   topology->connect_timeout_msec);

   bson_mutex_init (&topology->mutex);
   mongoc_cond_init (&topology->cond_client);
   mongoc_cond_init (&topology->cond_server);

   if (single_threaded) {
      /* single threaded clients negotiate sasl supported mechanisms during
       * a topology scan. */
      if (_mongoc_uri_requires_auth_negotiation (uri)) {
         topology->scanner->negotiate_sasl_supported_mechs = true;
      }
   }

   topology_valid = true;
   service = mongoc_uri_get_service (uri);
   if (service) {
      memset (&rr_data, 0, sizeof (mongoc_rr_data_t));

      /* a mongodb+srv URI. try SRV lookup, if no error then also try TXT */
      prefixed_service = bson_strdup_printf ("_mongodb._tcp.%s", service);
      if (!_mongoc_client_get_rr (prefixed_service,
                                  MONGOC_RR_SRV,
                                  topology->uri,
                                  &rr_data,
                                  &topology->scanner->error) ||
          !_mongoc_client_get_rr (service,
                                  MONGOC_RR_TXT,
                                  topology->uri,
                                  NULL,
                                  &topology->scanner->error)) {
         topology_valid = false;
      } else {
         topology->last_srv_scan = bson_get_monotonic_time ();
         topology->rescanSRVIntervalMS = BSON_MAX (
            rr_data.min_ttl * 1000, MONGOC_TOPOLOGY_MIN_RESCAN_SRV_INTERVAL_MS);
      }

      bson_free (prefixed_service);
   }

   /*
    * Set topology type from URI:
    *   - if we've got a replicaSet name, initialize to RS_NO_PRIMARY
    *   - otherwise, if the seed list has a single host, initialize to SINGLE
    *   - everything else gets initialized to UNKNOWN
    */
   hl = mongoc_uri_get_hosts (topology->uri);
   if (mongoc_uri_get_replica_set (topology->uri)) {
      init_type = MONGOC_TOPOLOGY_RS_NO_PRIMARY;
   } else {
      if (hl && hl->next) {
         init_type = MONGOC_TOPOLOGY_UNKNOWN;
      } else {
         init_type = MONGOC_TOPOLOGY_SINGLE;
      }
   }

   topology->description.type = init_type;

   if (!topology_valid) {
      /* add no nodes */
      return topology;
   }

   while (hl) {
      mongoc_topology_description_add_server (
         &topology->description, hl->host_and_port, &id);
      mongoc_topology_scanner_add (topology->scanner, hl, id);

      hl = hl->next;
   }

   return topology;
}
/*
 *-------------------------------------------------------------------------
 *
 * mongoc_topology_set_apm_callbacks --
 *
 *       Set Application Performance Monitoring callbacks.
 *
 *-------------------------------------------------------------------------
 */
void
mongoc_topology_set_apm_callbacks (mongoc_topology_t *topology,
                                   mongoc_apm_callbacks_t *callbacks,
                                   void *context)
{
   if (callbacks) {
      memcpy (&topology->description.apm_callbacks,
              callbacks,
              sizeof (mongoc_apm_callbacks_t));
      memcpy (&topology->scanner->apm_callbacks,
              callbacks,
              sizeof (mongoc_apm_callbacks_t));
   } else {
      memset (&topology->description.apm_callbacks,
              0,
              sizeof (mongoc_apm_callbacks_t));
      memset (
         &topology->scanner->apm_callbacks, 0, sizeof (mongoc_apm_callbacks_t));
   }

   topology->description.apm_context = context;
   topology->scanner->apm_context = context;
}

/*
 *-------------------------------------------------------------------------
 *
 * mongoc_topology_destroy --
 *
 *       Free the memory associated with this topology object.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @topology will be cleaned up.
 *
 *-------------------------------------------------------------------------
 */
void
mongoc_topology_destroy (mongoc_topology_t *topology)
{
   if (!topology) {
      return;
   }

#ifdef MONGOC_ENABLE_CLIENT_SIDE_ENCRYPTION
   bson_free (topology->keyvault_db);
   bson_free (topology->keyvault_coll);
   mongoc_client_destroy (topology->mongocryptd_client);
   mongoc_client_pool_destroy (topology->mongocryptd_client_pool);
   _mongoc_crypt_destroy (topology->crypt);
   bson_destroy (topology->mongocryptd_spawn_args);
   bson_free (topology->mongocryptd_spawn_path);
#endif

   _mongoc_topology_background_thread_stop (topology);
   _mongoc_topology_description_monitor_closed (&topology->description);

   mongoc_uri_destroy (topology->uri);
   mongoc_topology_description_destroy (&topology->description);
   mongoc_topology_scanner_destroy (topology->scanner);

   /* If we are single-threaded, the client will try to call
      _mongoc_topology_end_sessions_cmd when it dies. This removes
      sessions from the pool as it calls endSessions on them. In
      case this does not succeed, we clear the pool again here. */
   _mongoc_topology_clear_session_pool (topology);

   mongoc_cond_destroy (&topology->cond_client);
   mongoc_cond_destroy (&topology->cond_server);
   bson_mutex_destroy (&topology->mutex);

   bson_free (topology);
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_clear_session_pool --
 *
 *       Clears the pool of server sessions without sending endSessions.
 *
 * Returns:
 *       Nothing.
 *
 * Side effects:
 *       Server session pool will be emptied.
 *
 *--------------------------------------------------------------------------
 */

void
_mongoc_topology_clear_session_pool (mongoc_topology_t *topology)
{
   mongoc_server_session_t *ss, *tmp1, *tmp2;

   CDL_FOREACH_SAFE (topology->session_pool, ss, tmp1, tmp2)
   {
      _mongoc_server_session_destroy (ss);
   }
   topology->session_pool = NULL;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_rescan_srv --
 *
 *      Queries SRV records for new hosts in a mongos cluster.
 *
 *      NOTE: this method expects @topology's mutex to be locked on entry.
 *
 * --------------------------------------------------------------------------
 */
static void
mongoc_topology_rescan_srv (mongoc_topology_t *topology)
{
   mongoc_rr_data_t rr_data = {0};
   mongoc_host_list_t *h = NULL;
   const char *service;
   char *prefixed_service = NULL;
   int64_t scan_time;

   if ((topology->description.type != MONGOC_TOPOLOGY_SHARDED) &&
       (topology->description.type != MONGOC_TOPOLOGY_UNKNOWN)) {
      /* Only perform rescan for sharded topology. */
      return;
   }

   service = mongoc_uri_get_service (topology->uri);
   if (!service) {
      /* Only rescan if we have a mongodb+srv:// URI. */
      return;
   }

   scan_time = topology->last_srv_scan + (topology->rescanSRVIntervalMS * 1000);
   if (bson_get_monotonic_time () < scan_time) {
      /* Query SRV no more frequently than rescanSRVIntervalMS. */
      return;
   }

   /* Go forth and query... */
   rr_data.hosts =
      _mongoc_host_list_copy (mongoc_uri_get_hosts (topology->uri), NULL);

   prefixed_service = bson_strdup_printf ("_mongodb._tcp.%s", service);
   if (!_mongoc_client_get_rr (prefixed_service,
                               MONGOC_RR_SRV,
                               topology->uri,
                               &rr_data,
                               &topology->scanner->error)) {
      /* Failed querying, soldier on and try again next time. */
      topology->rescanSRVIntervalMS = topology->description.heartbeat_msec;
      GOTO (done);
   }

   topology->last_srv_scan = bson_get_monotonic_time ();
   topology->rescanSRVIntervalMS = BSON_MAX (
      rr_data.min_ttl * 1000, MONGOC_TOPOLOGY_MIN_RESCAN_SRV_INTERVAL_MS);

   if (rr_data.count == 0) {
      /* Special case when DNS returns zero records successfully.
       * Leave the toplogy alone and perform another scan at the next interval
       * rather than removing all records and having nothing to connect to.
       * For no verified hosts drivers "MUST temporarily set rescanSRVIntervalMS
       * to heartbeatFrequencyMS until at least one verified SRV record is
       * obtained."
       */
      topology->rescanSRVIntervalMS = topology->description.heartbeat_msec;
      GOTO (done);
   }

   /* rr_data.hosts was initialized to the current set of known hosts
    * on entry, and mongoc_client_get_rr will have stripped it down to
    * only include hosts which were NOT included in the most recent query.
    * Remove those hosts and we're left with only active servers.
    */
   for (h = rr_data.hosts; h; h = rr_data.hosts) {
      rr_data.hosts = h->next;
      mongoc_uri_remove_host (topology->uri, h->host, h->port);
      bson_free (h);
   }

done:
   bson_free (prefixed_service);
   _mongoc_host_list_destroy_all (rr_data.hosts);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scan_once --
 *
 *      Runs a single complete scan.
 *
 *      NOTE: this method expects @topology's mutex to be locked on entry.
 *
 *      NOTE: this method unlocks and re-locks @topology's mutex.
 *
 *--------------------------------------------------------------------------
 */
static void
mongoc_topology_scan_once (mongoc_topology_t *topology, bool obey_cooldown)
{
   /* Prior to scanning hosts, update the list of SRV hosts, if applicable. */
   mongoc_topology_rescan_srv (topology);

   /* since the last scan, members may be added or removed from the topology
    * description based on ismaster responses in connection handshakes, see
    * _mongoc_topology_update_from_handshake. retire scanner nodes for removed
    * members and create scanner nodes for new ones. */
   mongoc_topology_reconcile (topology);
   mongoc_topology_scanner_start (topology->scanner, obey_cooldown);

   /* scanning locks and unlocks the mutex itself until the scan is done */
   bson_mutex_unlock (&topology->mutex);
   mongoc_topology_scanner_work (topology->scanner);

   bson_mutex_lock (&topology->mutex);

   _mongoc_topology_scanner_finish (topology->scanner);

   topology->last_scan = bson_get_monotonic_time ();
   topology->stale = false;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_do_blocking_scan --
 *
 *       Monitoring entry for single-threaded use case. Assumes the caller
 *       has checked that it's the right time to scan.
 *
 *--------------------------------------------------------------------------
 */
void
_mongoc_topology_do_blocking_scan (mongoc_topology_t *topology,
                                   bson_error_t *error)
{
   topology->scanner_state = MONGOC_TOPOLOGY_SCANNER_SINGLE_THREADED;

   _mongoc_handshake_freeze ();

   bson_mutex_lock (&topology->mutex);
   mongoc_topology_scan_once (topology, true /* obey cooldown */);
   bson_mutex_unlock (&topology->mutex);
   mongoc_topology_scanner_get_error (topology->scanner, error);
}


bool
mongoc_topology_compatible (const mongoc_topology_description_t *td,
                            const mongoc_read_prefs_t *read_prefs,
                            bson_error_t *error)
{
   int64_t max_staleness_seconds;
   int32_t max_wire_version;

   if (td->compatibility_error.code) {
      if (error) {
         memcpy (error, &td->compatibility_error, sizeof (bson_error_t));
      }
      return false;
   }

   if (!read_prefs) {
      /* NULL means read preference Primary */
      return true;
   }

   max_staleness_seconds =
      mongoc_read_prefs_get_max_staleness_seconds (read_prefs);

   if (max_staleness_seconds != MONGOC_NO_MAX_STALENESS) {
      max_wire_version =
         mongoc_topology_description_lowest_max_wire_version (td);

      if (max_wire_version < WIRE_VERSION_MAX_STALENESS) {
         bson_set_error (error,
                         MONGOC_ERROR_COMMAND,
                         MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                         "Not all servers support maxStalenessSeconds");
         return false;
      }

      /* shouldn't happen if we've properly enforced wire version */
      if (!mongoc_topology_description_all_sds_have_write_date (td)) {
         bson_set_error (error,
                         MONGOC_ERROR_COMMAND,
                         MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                         "Not all servers have lastWriteDate");
         return false;
      }

      if (!_mongoc_topology_description_validate_max_staleness (
             td, max_staleness_seconds, error)) {
         return false;
      }
   }

   return true;
}


static void
_mongoc_server_selection_error (const char *msg,
                                const bson_error_t *scanner_error,
                                bson_error_t *error)
{
   if (scanner_error && scanner_error->code) {
      bson_set_error (error,
                      MONGOC_ERROR_SERVER_SELECTION,
                      MONGOC_ERROR_SERVER_SELECTION_FAILURE,
                      "%s: %s",
                      msg,
                      scanner_error->message);
   } else {
      bson_set_error (error,
                      MONGOC_ERROR_SERVER_SELECTION,
                      MONGOC_ERROR_SERVER_SELECTION_FAILURE,
                      "%s",
                      msg);
   }
}

/*
 *-------------------------------------------------------------------------
 *
 * mongoc_topology_select --
 *
 *       Selects a server description for an operation based on @optype
 *       and @read_prefs.
 *
 *       NOTE: this method returns a copy of the original server
 *       description. Callers must own and clean up this copy.
 *
 *       NOTE: this method locks and unlocks @topology's mutex.
 *
 * Parameters:
 *       @topology: The topology.
 *       @optype: Whether we are selecting for a read or write operation.
 *       @read_prefs: Required, the read preferences for the command.
 *       @error: Required, out pointer for error info.
 *
 * Returns:
 *       A mongoc_server_description_t, or NULL on failure, in which case
 *       @error will be set.
 *
 * Side effects:
 *       @error may be set.
 *
 *-------------------------------------------------------------------------
 */
mongoc_server_description_t *
mongoc_topology_select (mongoc_topology_t *topology,
                        mongoc_ss_optype_t optype,
                        const mongoc_read_prefs_t *read_prefs,
                        bson_error_t *error)
{
   uint32_t server_id =
      mongoc_topology_select_server_id (topology, optype, read_prefs, error);

   if (server_id) {
      /* new copy of the server description */
      return mongoc_topology_server_by_id (topology, server_id, error);
   } else {
      return NULL;
   }
}

/*
 *-------------------------------------------------------------------------
 *
 * mongoc_topology_select_server_id --
 *
 *       Alternative to mongoc_topology_select when you only need the id.
 *
 * Returns:
 *       A server id, or 0 on failure, in which case @error will be set.
 *
 *-------------------------------------------------------------------------
 */
uint32_t
mongoc_topology_select_server_id (mongoc_topology_t *topology,
                                  mongoc_ss_optype_t optype,
                                  const mongoc_read_prefs_t *read_prefs,
                                  bson_error_t *error)
{
   static const char *timeout_msg =
      "No suitable servers found: `serverSelectionTimeoutMS` expired";

   mongoc_topology_scanner_t *ts;
   int r;
   int64_t local_threshold_ms;
   mongoc_server_description_t *selected_server = NULL;
   bool try_once;
   int64_t sleep_usec;
   bool tried_once;
   bson_error_t scanner_error = {0};
   int64_t heartbeat_msec;
   uint32_t server_id;

   /* These names come from the Server Selection Spec pseudocode */
   int64_t loop_start;  /* when we entered this function */
   int64_t loop_end;    /* when we last completed a loop (single-threaded) */
   int64_t scan_ready;  /* the soonest we can do a blocking scan */
   int64_t next_update; /* the latest we must do a blocking scan */
   int64_t expire_at;   /* when server selection timeout expires */

   BSON_ASSERT (topology);
   ts = topology->scanner;

   bson_mutex_lock (&topology->mutex);
   /* It isn't strictly necessary to lock here, because if the topology
    * is invalid, it will never become valid. Lock anyway for consistency. */
   if (!mongoc_topology_scanner_valid (ts)) {
      if (error) {
         mongoc_topology_scanner_get_error (ts, error);
         error->domain = MONGOC_ERROR_SERVER_SELECTION;
         error->code = MONGOC_ERROR_SERVER_SELECTION_FAILURE;
      }
      bson_mutex_unlock (&topology->mutex);
      return 0;
   }
   bson_mutex_unlock (&topology->mutex);

   heartbeat_msec = topology->description.heartbeat_msec;
   local_threshold_ms = topology->local_threshold_msec;
   try_once = topology->server_selection_try_once;
   loop_start = loop_end = bson_get_monotonic_time ();
   expire_at =
      loop_start + ((int64_t) topology->server_selection_timeout_msec * 1000);

   if (topology->single_threaded) {
      _mongoc_topology_description_monitor_opening (&topology->description);

      tried_once = false;
      next_update = topology->last_scan + heartbeat_msec * 1000;
      if (next_update < loop_start) {
         /* we must scan now */
         topology->stale = true;
      }

      /* until we find a server or time out */
      for (;;) {
         if (topology->stale) {
            /* how soon are we allowed to scan? */
            scan_ready = topology->last_scan +
                         topology->min_heartbeat_frequency_msec * 1000;

            if (scan_ready > expire_at && !try_once) {
               /* selection timeout will expire before min heartbeat passes */
               _mongoc_server_selection_error (
                  "No suitable servers found: "
                  "`serverselectiontimeoutms` timed out",
                  &scanner_error,
                  error);

               return 0;
            }

            sleep_usec = scan_ready - loop_end;
            if (sleep_usec > 0) {
               if (try_once &&
                   mongoc_topology_scanner_in_cooldown (ts, scan_ready)) {
                  _mongoc_server_selection_error (
                     "No servers yet eligible for rescan",
                     &scanner_error,
                     error);

                  return 0;
               }

               _mongoc_usleep (sleep_usec);
            }

            /* takes up to connectTimeoutMS. sets "last_scan", clears "stale" */
            _mongoc_topology_do_blocking_scan (topology, &scanner_error);
            loop_end = topology->last_scan;
            tried_once = true;
         }

         if (!mongoc_topology_compatible (
                &topology->description, read_prefs, error)) {
            return 0;
         }

         selected_server = mongoc_topology_description_select (
            &topology->description, optype, read_prefs, local_threshold_ms);

         if (selected_server) {
            return selected_server->id;
         }

         topology->stale = true;

         if (try_once) {
            if (tried_once) {
               _mongoc_server_selection_error (
                  "No suitable servers found (`serverSelectionTryOnce` set)",
                  &scanner_error,
                  error);

               return 0;
            }
         } else {
            loop_end = bson_get_monotonic_time ();

            if (loop_end > expire_at) {
               /* no time left in server_selection_timeout_msec */
               _mongoc_server_selection_error (
                  timeout_msg, &scanner_error, error);

               return 0;
            }
         }
      }
   }

   /* With background thread */
   /* we break out when we've found a server or timed out */
   for (;;) {
      bson_mutex_lock (&topology->mutex);

      if (!mongoc_topology_compatible (
             &topology->description, read_prefs, error)) {
         bson_mutex_unlock (&topology->mutex);
         return 0;
      }

      selected_server = mongoc_topology_description_select (
         &topology->description, optype, read_prefs, local_threshold_ms);

      if (!selected_server) {
         _mongoc_topology_request_scan (topology);

         r = mongoc_cond_timedwait (&topology->cond_client,
                                    &topology->mutex,
                                    (expire_at - loop_start) / 1000);

         mongoc_topology_scanner_get_error (ts, &scanner_error);
         bson_mutex_unlock (&topology->mutex);

#ifdef _WIN32
         if (r == WSAETIMEDOUT) {
#else
         if (r == ETIMEDOUT) {
#endif
            /* handle timeouts */
            _mongoc_server_selection_error (timeout_msg, &scanner_error, error);

            return 0;
         } else if (r) {
            bson_set_error (error,
                            MONGOC_ERROR_SERVER_SELECTION,
                            MONGOC_ERROR_SERVER_SELECTION_FAILURE,
                            "Unknown error '%d' received while waiting on "
                            "thread condition",
                            r);
            return 0;
         }

         loop_start = bson_get_monotonic_time ();

         if (loop_start > expire_at) {
            _mongoc_server_selection_error (timeout_msg, &scanner_error, error);

            return 0;
         }
      } else {
         server_id = selected_server->id;
         bson_mutex_unlock (&topology->mutex);
         return server_id;
      }
   }
}

/*
 *-------------------------------------------------------------------------
 *
 * mongoc_topology_server_by_id --
 *
 *      Get the server description for @id, if that server is present
 *      in @description. Otherwise, return NULL and fill out the optional
 *      @error.
 *
 *      NOTE: this method returns a copy of the original server
 *      description. Callers must own and clean up this copy.
 *
 *      NOTE: this method locks and unlocks @topology's mutex.
 *
 * Returns:
 *      A mongoc_server_description_t, or NULL.
 *
 * Side effects:
 *      Fills out optional @error if server not found.
 *
 *-------------------------------------------------------------------------
 */

mongoc_server_description_t *
mongoc_topology_server_by_id (mongoc_topology_t *topology,
                              uint32_t id,
                              bson_error_t *error)
{
   mongoc_server_description_t *sd;

   bson_mutex_lock (&topology->mutex);

   sd = mongoc_server_description_new_copy (
      mongoc_topology_description_server_by_id (
         &topology->description, id, error));

   bson_mutex_unlock (&topology->mutex);

   return sd;
}

/*
 *-------------------------------------------------------------------------
 *
 * mongoc_topology_host_by_id --
 *
 *      Copy the mongoc_host_list_t for @id, if that server is present
 *      in @description. Otherwise, return NULL and fill out the optional
 *      @error.
 *
 *      NOTE: this method returns a copy of the original mongoc_host_list_t.
 *      Callers must own and clean up this copy.
 *
 *      NOTE: this method locks and unlocks @topology's mutex.
 *
 * Returns:
 *      A mongoc_host_list_t, or NULL.
 *
 * Side effects:
 *      Fills out optional @error if server not found.
 *
 *-------------------------------------------------------------------------
 */

mongoc_host_list_t *
_mongoc_topology_host_by_id (mongoc_topology_t *topology,
                             uint32_t id,
                             bson_error_t *error)
{
   mongoc_server_description_t *sd;
   mongoc_host_list_t *host = NULL;

   bson_mutex_lock (&topology->mutex);

   /* not a copy - direct pointer into topology description data */
   sd = mongoc_topology_description_server_by_id (
      &topology->description, id, error);

   if (sd) {
      host = bson_malloc0 (sizeof (mongoc_host_list_t));
      memcpy (host, &sd->host, sizeof (mongoc_host_list_t));
   }

   bson_mutex_unlock (&topology->mutex);

   return host;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_request_scan --
 *
 *       Non-locking variant
 *
 *--------------------------------------------------------------------------
 */

void
_mongoc_topology_request_scan (mongoc_topology_t *topology)
{
   topology->scan_requested = true;

   mongoc_cond_signal (&topology->cond_server);
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_invalidate_server --
 *
 *      Invalidate the given server after receiving a network error in
 *      another part of the client.
 *
 *      NOTE: this method uses @topology's mutex.
 *
 *--------------------------------------------------------------------------
 */
void
mongoc_topology_invalidate_server (mongoc_topology_t *topology,
                                   uint32_t id,
                                   const bson_error_t *error)
{
   BSON_ASSERT (error);

   bson_mutex_lock (&topology->mutex);
   mongoc_topology_description_invalidate_server (
      &topology->description, id, error);
   bson_mutex_unlock (&topology->mutex);
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_update_from_handshake --
 *
 *      A client opens a new connection and calls ismaster on it when it
 *      detects a closed connection in _mongoc_cluster_check_interval, or if
 *      mongoc_client_pool_pop creates a new client. Update the topology
 *      description from the ismaster response.
 *
 *      NOTE: this method uses @topology's mutex.
 *
 * Returns:
 *      false if the server was removed from the topology
 *--------------------------------------------------------------------------
 */
bool
_mongoc_topology_update_from_handshake (mongoc_topology_t *topology,
                                        const mongoc_server_description_t *sd)
{
   bool has_server;

   BSON_ASSERT (topology);
   BSON_ASSERT (sd);

   bson_mutex_lock (&topology->mutex);

   /* return false if server was removed from topology */
   has_server = _mongoc_topology_update_no_lock (
      sd->id, &sd->last_is_master, sd->round_trip_time_msec, topology, NULL);

   /* if pooled, wake threads waiting in mongoc_topology_server_by_id */
   mongoc_cond_broadcast (&topology->cond_client);
   bson_mutex_unlock (&topology->mutex);

   return has_server;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_update_last_used --
 *
 *       Internal function. In single-threaded mode only, track when the socket
 *       to a particular server was last used. This is required for
 *       mongoc_cluster_check_interval to know when a socket has been idle.
 *
 *--------------------------------------------------------------------------
 */

void
_mongoc_topology_update_last_used (mongoc_topology_t *topology,
                                   uint32_t server_id)
{
   mongoc_topology_scanner_node_t *node;

   if (!topology->single_threaded) {
      return;
   }

   node = mongoc_topology_scanner_get_node (topology->scanner, server_id);
   if (node) {
      node->last_used = bson_get_monotonic_time ();
   }
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_server_timestamp --
 *
 *      Return the topology's scanner's timestamp for the given server,
 *      or -1 if there is no scanner node for the given server.
 *
 *      NOTE: this method uses @topology's mutex.
 *
 * Returns:
 *      Timestamp, or -1
 *
 *--------------------------------------------------------------------------
 */
int64_t
mongoc_topology_server_timestamp (mongoc_topology_t *topology, uint32_t id)
{
   mongoc_topology_scanner_node_t *node;
   int64_t timestamp = -1;

   bson_mutex_lock (&topology->mutex);

   node = mongoc_topology_scanner_get_node (topology->scanner, id);
   if (node) {
      timestamp = node->timestamp;
   }

   bson_mutex_unlock (&topology->mutex);

   return timestamp;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_get_type --
 *
 *      Return the topology's description's type.
 *
 *      NOTE: this method uses @topology's mutex.
 *
 * Returns:
 *      The topology description type.
 *
 *--------------------------------------------------------------------------
 */
mongoc_topology_description_type_t
_mongoc_topology_get_type (mongoc_topology_t *topology)
{
   mongoc_topology_description_type_t td_type;

   bson_mutex_lock (&topology->mutex);

   td_type = topology->description.type;

   bson_mutex_unlock (&topology->mutex);

   return td_type;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_run_background --
 *
 *       The background topology monitoring thread runs in this loop.
 *
 *       NOTE: this method uses @topology's mutex.
 *
 *--------------------------------------------------------------------------
 */
static void *
_mongoc_topology_run_background (void *data)
{
   mongoc_topology_t *topology;
   int64_t now;
   int64_t last_scan;
   int64_t timeout;
   int64_t force_timeout;
   int64_t heartbeat_msec;
   int r;

   BSON_ASSERT (data);

   last_scan = 0;
   topology = (mongoc_topology_t *) data;
   heartbeat_msec = topology->description.heartbeat_msec;

   /* we exit this loop when shutting down, or on error */
   for (;;) {
      /* unlocked after starting a scan or after breaking out of the loop */
      bson_mutex_lock (&topology->mutex);
      if (!mongoc_topology_scanner_valid (topology->scanner)) {
         bson_mutex_unlock (&topology->mutex);
         goto DONE;
      }

      /* we exit this loop on error, or when we should scan immediately */
      for (;;) {
         if (topology->scanner_state == MONGOC_TOPOLOGY_SCANNER_SHUTTING_DOWN) {
            bson_mutex_unlock (&topology->mutex);
            goto DONE;
         }

         now = bson_get_monotonic_time ();

         if (last_scan == 0) {
            /* set up the "last scan" as exactly long enough to force an
             * immediate scan on the first pass */
            last_scan = now - (heartbeat_msec * 1000);
         }

         timeout = heartbeat_msec - ((now - last_scan) / 1000);

         /* if someone's specifically asked for a scan, use a shorter interval
          */
         if (topology->scan_requested) {
            force_timeout = topology->min_heartbeat_frequency_msec -
                            ((now - last_scan) / 1000);

            timeout = BSON_MIN (timeout, force_timeout);
         }

         /* if we can start scanning, do so immediately */
         if (timeout <= 0) {
            break;
         } else {
            /* otherwise wait until someone:
             *   o requests a scan
             *   o we time out
             *   o requests a shutdown
             */
            r = mongoc_cond_timedwait (
               &topology->cond_server, &topology->mutex, timeout);

#ifdef _WIN32
            if (!(r == 0 || r == WSAETIMEDOUT)) {
#else
            if (!(r == 0 || r == ETIMEDOUT)) {
#endif
               bson_mutex_unlock (&topology->mutex);
               /* handle errors */
               goto DONE;
            }

            /* if we timed out, or were woken up, check if it's time to scan
             * again, or bail out */
         }
      }

      topology->scan_requested = false;
      mongoc_topology_scan_once (topology, false /* obey cooldown */);
      bson_mutex_unlock (&topology->mutex);

      last_scan = bson_get_monotonic_time ();
   }

DONE:
   return NULL;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_start_background_scanner
 *
 *       Start the topology background thread running. This should only be
 *       called once per pool. If clients are created separately (not
 *       through a pool) the SDAM logic will not be run in a background
 *       thread. Returns whether or not the scanner is running on termination
 *       of the function.
 *
 *       NOTE: this method uses @topology's mutex.
 *
 *--------------------------------------------------------------------------
 */

bool
_mongoc_topology_start_background_scanner (mongoc_topology_t *topology)
{
   int r;

   if (topology->single_threaded) {
      return false;
   }

   bson_mutex_lock (&topology->mutex);

   if (topology->scanner_state == MONGOC_TOPOLOGY_SCANNER_BG_RUNNING) {
      bson_mutex_unlock (&topology->mutex);
      return true;
   }

   BSON_ASSERT (topology->scanner_state == MONGOC_TOPOLOGY_SCANNER_OFF);

   topology->scanner_state = MONGOC_TOPOLOGY_SCANNER_BG_RUNNING;

   _mongoc_handshake_freeze ();
   _mongoc_topology_description_monitor_opening (&topology->description);

   r = bson_thread_create (
      &topology->thread, _mongoc_topology_run_background, topology);

   if (r != 0) {
      MONGOC_ERROR ("could not start topology scanner thread: %s",
                    strerror (r));
      abort ();
   }

   bson_mutex_unlock (&topology->mutex);

   return true;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_background_thread_stop --
 *
 *       Stop the topology background thread. Called by the owning pool at
 *       its destruction.
 *
 *       NOTE: this method uses @topology's mutex.
 *
 *--------------------------------------------------------------------------
 */

void
_mongoc_topology_background_thread_stop (mongoc_topology_t *topology)
{
   bool join_thread = false;

   if (topology->single_threaded) {
      return;
   }

   bson_mutex_lock (&topology->mutex);

   BSON_ASSERT (topology->scanner_state !=
                MONGOC_TOPOLOGY_SCANNER_SHUTTING_DOWN);

   if (topology->scanner_state == MONGOC_TOPOLOGY_SCANNER_BG_RUNNING) {
      /* if the background thread is running, request a shutdown and signal the
       * thread */
      topology->scanner_state = MONGOC_TOPOLOGY_SCANNER_SHUTTING_DOWN;
      mongoc_cond_signal (&topology->cond_server);
      join_thread = true;
   } else {
      /* nothing to do if it's already off */
   }

   bson_mutex_unlock (&topology->mutex);

   if (join_thread) {
      /* if we're joining the thread, wait for it to come back and broadcast
       * all listeners */
      bson_thread_join (topology->thread);

      bson_mutex_lock (&topology->mutex);
      topology->scanner_state = MONGOC_TOPOLOGY_SCANNER_OFF;
      bson_mutex_unlock (&topology->mutex);

      mongoc_cond_broadcast (&topology->cond_client);
   }
}

bool
_mongoc_topology_set_appname (mongoc_topology_t *topology, const char *appname)
{
   bool ret = false;
   bson_mutex_lock (&topology->mutex);

   if (topology->scanner_state == MONGOC_TOPOLOGY_SCANNER_OFF) {
      ret = _mongoc_topology_scanner_set_appname (topology->scanner, appname);
   } else {
      MONGOC_ERROR ("Cannot set appname after handshake initiated");
   }
   bson_mutex_unlock (&topology->mutex);
   return ret;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_update_cluster_time --
 *
 *       Internal function. If the server reply has a later $clusterTime than
 *       any seen before, update the topology's clusterTime. See the Driver
 *       Sessions Spec.
 *
 *--------------------------------------------------------------------------
 */

void
_mongoc_topology_update_cluster_time (mongoc_topology_t *topology,
                                      const bson_t *reply)
{
   bson_mutex_lock (&topology->mutex);
   mongoc_topology_description_update_cluster_time (&topology->description,
                                                    reply);
   _mongoc_topology_scanner_set_cluster_time (
      topology->scanner, &topology->description.cluster_time);
   bson_mutex_unlock (&topology->mutex);
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_pop_server_session --
 *
 *       Internal function. Get a server session from the pool or create
 *       one. On error, return NULL and fill out @error.
 *
 *--------------------------------------------------------------------------
 */

mongoc_server_session_t *
_mongoc_topology_pop_server_session (mongoc_topology_t *topology,
                                     bson_error_t *error)
{
   int64_t timeout;
   mongoc_server_session_t *ss = NULL;
   mongoc_topology_description_t *td;

   ENTRY;

   bson_mutex_lock (&topology->mutex);

   td = &topology->description;
   timeout = td->session_timeout_minutes;

   if (timeout == MONGOC_NO_SESSIONS) {
      /* if needed, connect and check for session timeout again */
      if (!mongoc_topology_description_has_data_node (td)) {
         bson_mutex_unlock (&topology->mutex);
         if (!mongoc_topology_select_server_id (
                topology, MONGOC_SS_READ, NULL, error)) {
            RETURN (NULL);
         }

         bson_mutex_lock (&topology->mutex);
         timeout = td->session_timeout_minutes;
      }

      if (timeout == MONGOC_NO_SESSIONS) {
         bson_mutex_unlock (&topology->mutex);
         bson_set_error (error,
                         MONGOC_ERROR_CLIENT,
                         MONGOC_ERROR_CLIENT_SESSION_FAILURE,
                         "Server does not support sessions");
         RETURN (NULL);
      }
   }

   while (topology->session_pool) {
      ss = topology->session_pool;
      CDL_DELETE (topology->session_pool, ss);
      if (_mongoc_server_session_timed_out (ss, timeout)) {
         _mongoc_server_session_destroy (ss);
         ss = NULL;
      } else {
         break;
      }
   }

   bson_mutex_unlock (&topology->mutex);

   if (!ss) {
      ss = _mongoc_server_session_new (error);
   }

   RETURN (ss);
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_push_server_session --
 *
 *       Internal function. Return a server session to the pool.
 *
 *--------------------------------------------------------------------------
 */

void
_mongoc_topology_push_server_session (mongoc_topology_t *topology,
                                      mongoc_server_session_t *server_session)
{
   int64_t timeout;
   mongoc_server_session_t *ss;

   ENTRY;

   bson_mutex_lock (&topology->mutex);

   timeout = topology->description.session_timeout_minutes;

   /* start at back of queue and reap timed-out sessions */
   while (topology->session_pool && topology->session_pool->prev) {
      ss = topology->session_pool->prev;
      if (_mongoc_server_session_timed_out (ss, timeout)) {
         BSON_ASSERT (ss->next); /* silences clang scan-build */
         CDL_DELETE (topology->session_pool, ss);
         _mongoc_server_session_destroy (ss);
      } else {
         /* if ss is not timed out, sessions in front of it are ok too */
         break;
      }
   }

   /* If session is expiring or "dirty" (a network error occurred on it), do not
    * return it to the pool. */
   if (_mongoc_server_session_timed_out (server_session, timeout) ||
       server_session->dirty) {
      _mongoc_server_session_destroy (server_session);
   } else {
      /* silences clang scan-build */
      BSON_ASSERT (!topology->session_pool || (topology->session_pool->next &&
                                               topology->session_pool->prev));
      CDL_PREPEND (topology->session_pool, server_session);
   }

   bson_mutex_unlock (&topology->mutex);

   EXIT;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_end_sessions_cmd --
 *
 *       Internal function. End up to 10,000 server sessions. @cmd is an
 *       uninitialized document. Sessions are destroyed as their ids are
 *       appended to @cmd.
 *
 *       Driver Sessions Spec: "If the number of sessions is very large the
 *       endSessions command SHOULD be run multiple times to end 10,000
 *       sessions at a time (in order to avoid creating excessively large
 *       commands)."
 *
 * Returns:
 *      true if any session ids were appended to @cmd.
 *
 *--------------------------------------------------------------------------
 */

bool
_mongoc_topology_end_sessions_cmd (mongoc_topology_t *topology, bson_t *cmd)
{
   mongoc_server_session_t *ss, *tmp1, *tmp2;
   char buf[16];
   const char *key;
   uint32_t i;
   bson_t ar;

   bson_init (cmd);
   BSON_APPEND_ARRAY_BEGIN (cmd, "endSessions", &ar);

   i = 0;
   CDL_FOREACH_SAFE (topology->session_pool, ss, tmp1, tmp2)
   {
      bson_uint32_to_string (i, &key, buf, sizeof buf);
      BSON_APPEND_DOCUMENT (&ar, key, &ss->lsid);
      CDL_DELETE (topology->session_pool, ss);
      _mongoc_server_session_destroy (ss);

      if (++i == 10000) {
         break;
      }
   }

   bson_append_array_end (cmd, &ar);

   return i > 0;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_get_ismaster --
 *
 *       Locks topology->mutex and retrieves (possibly constructing) the
 *       handshake on the topology scanner.
 *
 * Returns:
 *      A bson_t representing an ismaster command.
 *
 *--------------------------------------------------------------------------
 */
const bson_t *
_mongoc_topology_get_ismaster (mongoc_topology_t *topology)
{
   const bson_t *cmd;
   bson_mutex_lock (&topology->mutex);
   cmd = _mongoc_topology_scanner_get_ismaster (topology->scanner);
   bson_mutex_unlock (&topology->mutex);
   return cmd;
}

void
_mongoc_topology_bypass_cooldown (mongoc_topology_t *topology)
{
   BSON_ASSERT (topology->single_threaded);
   topology->scanner->bypass_cooldown = true;
}