#include <Rinternals.h>
#include <bson.h>
#include <mongoc.h>
#include <utils.h>

SEXP R_mongo_collection_drop (SEXP ptr);
SEXP R_mongo_collection_name (SEXP ptr);
SEXP R_mongo_collection_count (SEXP ptr, SEXP query);
SEXP R_mongo_collection_insert(SEXP ptr_col, SEXP ptr_bson, SEXP stop_on_error);
SEXP R_mongo_collection_create_index(SEXP ptr, SEXP keys);
SEXP R_mongo_collection_remove(SEXP ptr_col, SEXP ptr_bson, SEXP all);

SEXP R_mongo_collection_drop (SEXP ptr){
  mongoc_collection_t *col = r2col(ptr);
  bson_error_t err;

  if(!mongoc_collection_drop(col, &err))
    error(err.message);

  return ScalarLogical(1);
}

SEXP R_mongo_collection_name (SEXP ptr){
  mongoc_collection_t *col = r2col(ptr);
  const char * name = mongoc_collection_get_name(col);
  return mkStringUTF8(name);
}

SEXP R_mongo_collection_count (SEXP ptr, SEXP query){
  mongoc_collection_t *col = r2col(ptr);

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

SEXP R_mongo_collection_insert(SEXP ptr_col, SEXP ptr_bson, SEXP stop_on_error){
  mongoc_collection_t *col = r2col(ptr_col);
  bson_t *b = r2bson(ptr_bson);
  mongoc_insert_flags_t flags = asLogical(stop_on_error) ? MONGOC_INSERT_NONE : MONGOC_INSERT_CONTINUE_ON_ERROR;
  bson_error_t err;

  if(!mongoc_collection_insert(col, flags, b, NULL, &err))
    error(err.message);

  return ScalarLogical(1);
}

SEXP R_mongo_collection_create_index(SEXP ptr_col, SEXP ptr_bson) {
  mongoc_collection_t *col = r2col(ptr_col);
  bson_t *b = r2bson(ptr_bson);
  const mongoc_index_opt_t *options = mongoc_index_opt_get_default();
  bson_error_t err;

  if(!mongoc_collection_create_index(col, b, options, &err))
    error(err.message);

  return ScalarLogical(1);
}

SEXP R_mongo_collection_remove(SEXP ptr_col, SEXP ptr_bson, SEXP all){
  mongoc_collection_t *col = r2col(ptr_col);
  bson_t *b = r2bson(ptr_bson);
  bson_error_t err;
  mongoc_remove_flags_t flags = asLogical(all) ? MONGOC_REMOVE_NONE : MONGOC_REMOVE_SINGLE_REMOVE;

  if(!mongoc_collection_remove(col, flags, b, NULL, &err))
    error(err.message);

  return ScalarLogical(1);
}

SEXP R_mongo_collection_find(SEXP ptr_col, SEXP ptr_bson){
  return R_NilValue;
}