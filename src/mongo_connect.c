#include <Rinternals.h>
#include <bson.h>
#include <mongoc.h>

SEXP R_mongo_connect(SEXP uri_string, SEXP db, SEXP collection);
void fin_mongo (SEXP ptr);

SEXP R_mongo_connect(SEXP uri_string, SEXP db, SEXP collection) {
    mongoc_client_t *client;
    mongoc_collection_t *col;

    client = mongoc_client_new (CHAR(asChar(uri_string)));
    if(!client){
      error("Invalid uri_string. Try mongodb://localhost");
    }

    col = mongoc_client_get_collection (client, CHAR(asChar(db)), CHAR(asChar(collection)));
    SEXP ptr = PROTECT(R_MakeExternalPtr(col, R_NilValue, R_NilValue));

    //clean up on garbage collection
    R_RegisterCFinalizerEx(ptr, fin_mongo, 1);
    UNPROTECT(1);
    return ptr;
}

void fin_mongo(SEXP ptr){
  Rprintf("DEBUG: Destorying collection.\n");
  if(!R_ExternalPtrAddr(ptr)) return;
  mongoc_collection_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}
