#include <Rinternals.h>
#include <bson.h>
#include <mongoc.h>

SEXP R_mongo_collection_drop (SEXP ptr);
SEXP R_mongo_collection_name (SEXP ptr);

SEXP R_mongo_collection_drop (SEXP ptr){
  mongoc_collection_t *col = R_ExternalPtrAddr(ptr);
  bson_error_t err;

  if(!col) {
    error("Collection is null.");
  }

  int success = mongoc_collection_drop(col, &err);
  if(!success) {
    error(err.message);
  }
  return ScalarLogical(1);
}

SEXP R_mongo_collection_name (SEXP ptr){
  mongoc_collection_t *col = R_ExternalPtrAddr(ptr);
  if(!col) {
    error("Collection is null.");
  }
  const char * name = mongoc_collection_get_name(col);
  return mkString(name);
}

SEXP R_mongo_collection_count (SEXP ptr, SEXP query){
  mongoc_collection_t *col = R_ExternalPtrAddr(ptr);
  if(!col)
    error("Collection is null.");

  if (!isString(query))
    error("Argument query must be a string.");

  bson_error_t err;
  bson_t b_query;
  int success = bson_init_from_json(&b_query, translateCharUTF8(asChar(query)), -1, &err);
  if(!success){
    error(err.message);
  }

  int64_t count = mongoc_collection_count (col, MONGOC_QUERY_NONE, &b_query, 0, 0, NULL, &err);
  if (count < 0) {
    error(err.message);
  }
  return ScalarReal((double) count);
}