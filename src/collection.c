#include <Rinternals.h>
#include <bson.h>
#include <mongoc.h>
#include <utils.h>

SEXP R_mongo_collection_drop (SEXP ptr);
SEXP R_mongo_collection_name (SEXP ptr);
SEXP R_mongo_collection_count (SEXP ptr, SEXP query);
SEXP R_mongo_collection_create_index(SEXP ptr, SEXP keys);

SEXP R_mongo_collection_drop (SEXP ptr){
  mongoc_collection_t *col = R_ExternalPtrAddr(ptr);
  bson_error_t err;

  if(!col)
    error("Collection is null.");

  if(!mongoc_collection_drop(col, &err))
    error(err.message);

  return ScalarLogical(1);
}

SEXP R_mongo_collection_name (SEXP ptr){
  mongoc_collection_t *col = R_ExternalPtrAddr(ptr);
  if(!col)
    error("Collection is null.");

  const char * name = mongoc_collection_get_name(col);
  return mkStringUTF8(name);
}

SEXP R_mongo_collection_count (SEXP ptr, SEXP query){
  mongoc_collection_t *col = R_ExternalPtrAddr(ptr);
  if(!col)
    error("Collection is null.");

  if (!isString(query))
    error("Argument query must be a string.");

  bson_error_t err;
  bson_t b_query;
  if(!bson_init_from_json(&b_query, translateCharUTF8(asChar(query)), -1, &err))
    error(err.message);

  int64_t count = mongoc_collection_count (col, MONGOC_QUERY_NONE, &b_query, 0, 0, NULL, &err);
  if (count < 0)
    error(err.message);

  //R does not support int64
  return ScalarReal((double) count);
}