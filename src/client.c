#include <mongolite.h>

SEXP R_mongo_client_server_status(SEXP ptr_client) {
  bson_t reply;
  bson_error_t err;
  mongoc_client_t *client = r2client(ptr_client);
  if(!mongoc_client_get_server_status(client, NULL, &reply, &err))
    stop(err.message);
  return bson2list(&reply);
}

SEXP R_mongo_client_new(SEXP uri_string) {
  mongoc_client_t *client = mongoc_client_new (translateCharUTF8(asChar(uri_string)));
  if(!client)
    stop("Invalid uri_string. Try mongodb://localhost");

  //set ssl certificates here
  mongoc_client_set_ssl_opts(client, mongoc_ssl_opt_get_default());

  //verify that server is online
  bson_error_t err;
  if(!mongoc_client_get_server_status(client, NULL, NULL, &err)){
    mongoc_client_destroy(client);
    stop(err.message);
  }

  return client2r(client);
}
