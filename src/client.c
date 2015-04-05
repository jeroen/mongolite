#include <Rinternals.h>
#include <bson.h>
#include <mongoc.h>
#include <utils.h>

SEXP R_mongo_client_server_status(SEXP ptr_client) {
  bson_t reply;
  bson_error_t err;
  mongoc_client_t *client = r2client(ptr_client);
  if(!mongoc_client_get_server_status(client, NULL, &reply, &err))
    error(err.message);
  return bson2list(&reply);
}
