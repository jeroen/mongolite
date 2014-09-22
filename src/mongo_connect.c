#include <Rinternals.h>
#include <bson.h>
#include <mongoc.h>

SEXP R_mongo_connect(SEXP uri_string, SEXP db, SEXP collection) {
    mongoc_client_t *client;
    mongoc_collection_t *col;

    client = mongoc_client_new (CHAR(asChar(uri_string)));
    if(!client){
      error("Invalid uri_string.");
    }

    col = mongoc_client_get_collection (client, CHAR(asChar(db)), CHAR(asChar(collection)));
    return R_MakeExternalPtr(col, R_NilValue, R_NilValue);
}
