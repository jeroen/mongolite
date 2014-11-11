#include <Rinternals.h>
#include <bson.h>
#include <mongoc.h>
#include <utils.h>

SEXP R_mongo_connect(SEXP uri_string, SEXP db, SEXP collection);

SEXP R_mongo_connect(SEXP uri_string, SEXP db, SEXP collection) {
    mongoc_client_t *client;
    mongoc_collection_t *col;

    client = mongoc_client_new (translateCharUTF8(asChar(uri_string)));
    if(!client)
      error("Invalid uri_string. Try mongodb://localhost");

    col = mongoc_client_get_collection (client, translateCharUTF8(asChar(db)), translateCharUTF8(asChar(collection)));
    return(col2r(col));
}
