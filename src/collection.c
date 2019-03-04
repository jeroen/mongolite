#include <mongolite.h>

SEXP R_mongo_collection_new(SEXP ptr_client, SEXP collection, SEXP db) {
  mongoc_client_t *client = r2client(ptr_client);
  mongoc_collection_t *col = mongoc_client_get_collection (client,
    Rf_translateCharUTF8(Rf_asChar(db)), Rf_translateCharUTF8(Rf_asChar(collection)));
  return col2r(col, ptr_client);
}

SEXP R_mongo_get_default_database(SEXP ptr_client) {
  mongoc_client_t *client = r2client(ptr_client);
  mongoc_database_t *db = mongoc_client_get_default_database(client);
  if(db){
    SEXP out = PROTECT(Rf_mkString(mongoc_database_get_name(db)));
    mongoc_database_destroy(db);
    UNPROTECT(1);
    return out;
  } else {
    return R_NilValue;
  }
}

SEXP R_mongo_collection_drop (SEXP ptr){
  mongoc_collection_t *col = r2col(ptr);
  bson_error_t err;

  int res = mongoc_collection_drop(col, &err);
  if(!res && err.code != 26)
    stop(err.message);

  return Rf_ScalarLogical(res);
}

SEXP R_mongo_collection_name (SEXP ptr){
  mongoc_collection_t *col = r2col(ptr);
  const char * name = mongoc_collection_get_name(col);
  return mkStringUTF8(name);
}

SEXP R_mongo_collection_count (SEXP ptr, SEXP ptr_filter){
  mongoc_collection_t *col = r2col(ptr);
  bson_t *filter = r2bson(ptr_filter);
  bson_error_t err;
  int64_t count = mongoc_collection_count_documents (col, filter, NULL, NULL, NULL, &err);
  if (count < 0)
    stop(err.message);

  //R does not support int64
  return Rf_ScalarReal((double) count);
}

SEXP R_mongo_collection_insert_bson(SEXP ptr_col, SEXP ptr_bson, SEXP stop_on_error){
  mongoc_collection_t *col = r2col(ptr_col);
  bson_t *b = r2bson(ptr_bson);
  mongoc_insert_flags_t flags = Rf_asLogical(stop_on_error) ? MONGOC_INSERT_NONE : MONGOC_INSERT_CONTINUE_ON_ERROR;
  bson_error_t err;

  if(!mongoc_collection_insert(col, flags, b, NULL, &err))
    stop(err.message);

  return Rf_ScalarLogical(1);
}

SEXP R_mongo_collection_update(SEXP ptr_col, SEXP ptr_selector, SEXP ptr_update, SEXP ptr_filters, SEXP upsert, SEXP multiple, SEXP replace){
  mongoc_collection_t *col = r2col(ptr_col);
  bson_t *selector = r2bson(ptr_selector);
  bson_t *update = r2bson(ptr_update);

  bool success;
  bson_t opts;
  bson_init (&opts);
  BSON_APPEND_BOOL (&opts, "upsert", Rf_asLogical(upsert));
  if(!Rf_isNull(ptr_filters))
    BSON_APPEND_ARRAY (&opts, "arrayFilters",  r2bson(ptr_filters));

  bson_error_t err;
  bson_t reply;
  if(Rf_asLogical(replace)){
    success = mongoc_collection_replace_one(col, selector, update, &opts, &reply, &err);
  } else {
    success = Rf_asLogical(multiple) ?
      mongoc_collection_update_many(col, selector, update, &opts, &reply, &err) :
      mongoc_collection_update_one(col, selector, update, &opts, &reply, &err);
  }

  if(!success)
    stop(err.message);

  return bson2list(&reply);
}

SEXP R_mongo_collection_insert_page(SEXP ptr_col, SEXP json_vec, SEXP stop_on_error){
  if(!Rf_isString(json_vec) || !Rf_length(json_vec))
    stop("json_vec must be character string of at least length 1");

  //ordered means serial execution
  bool ordered = Rf_asLogical(stop_on_error);

  //create bulk operation
  bson_error_t err;
  bson_t *b;
  bson_t reply;
  mongoc_bulk_operation_t *bulk = mongoc_collection_create_bulk_operation_with_opts (r2col(ptr_col), NULL);
  for(int i = 0; i < Rf_length(json_vec); i++){
    b = bson_new_from_json ((uint8_t*) Rf_translateCharUTF8(Rf_asChar(STRING_ELT(json_vec, i))), -1, &err);
    if(!b){
      mongoc_bulk_operation_destroy (bulk);
      stop(err.message);
    }
    mongoc_bulk_operation_insert(bulk, b);
    bson_destroy (b);
    b = NULL;
  }

  //execute bulk operation
  bool success = mongoc_bulk_operation_execute (bulk, &reply, &err);
  mongoc_bulk_operation_destroy (bulk);

  //check for errors
  if(!success){
    if(ordered){
      Rf_errorcall(R_NilValue, err.message);
    } else {
      Rf_warningcall(R_NilValue, "Not all inserts were successful: %s\n", err.message);
    }
  }

  //get output
  SEXP out = PROTECT(bson2list(&reply));
  bson_destroy (&reply);
  UNPROTECT(1);
  return out;
}

SEXP R_mongo_collection_create_index(SEXP ptr_col, SEXP ptr_bson) {
  mongoc_collection_t *col = r2col(ptr_col);
  bson_t *keys = r2bson(ptr_bson);
  const char * collection_name = mongoc_collection_get_name(col);
  char * index_name = mongoc_collection_keys_to_index_string (keys);
  bson_error_t err;

  //From: https://s3.amazonaws.com/mciuploads/mongo-c-driver/docs/latest/create-indexes.html
  bson_t * command = BCON_NEW ("createIndexes", BCON_UTF8 (collection_name), "indexes",
    "[", "{", "key", BCON_DOCUMENT (keys), "name", BCON_UTF8 (index_name), "}", "]");

  if(!mongoc_collection_write_command_with_opts(col, command, NULL, NULL, &err))
    stop(err.message);

  return Rf_ScalarLogical(1);
}

SEXP R_mongo_collection_drop_index(SEXP ptr_col, SEXP name) {
  mongoc_collection_t *col = r2col(ptr_col);
  const char *str =  Rf_translateCharUTF8(Rf_asChar(name));
  bson_error_t err;

  if(!mongoc_collection_drop_index(col, str, &err))
    stop(err.message);

  return Rf_ScalarLogical(1);
}

SEXP R_mongo_collection_remove(SEXP ptr_col, SEXP ptr_bson, SEXP just_one){
  mongoc_collection_t *col = r2col(ptr_col);
  bson_t *b = r2bson(ptr_bson);
  bson_error_t err;
  mongoc_remove_flags_t flags = Rf_asLogical(just_one) ? MONGOC_REMOVE_SINGLE_REMOVE : MONGOC_REMOVE_NONE;

  if(!mongoc_collection_remove(col, flags, b, NULL, &err))
    stop(err.message);

  return Rf_ScalarLogical(1);
}

SEXP R_mongo_collection_find(SEXP ptr_col, SEXP ptr_query, SEXP ptr_opts) {
  mongoc_collection_t *col = r2col(ptr_col);
  bson_t *query = r2bson(ptr_query);
  bson_t *opts = r2bson(ptr_opts);
  mongoc_cursor_t *c = mongoc_collection_find_with_opts(col, query, opts, NULL);
  return cursor2r(c, ptr_col);
}

SEXP R_mongo_collection_find_indexes(SEXP ptr_col) {
  mongoc_collection_t *col = r2col(ptr_col);
  bson_error_t err;

  mongoc_cursor_t *c = mongoc_collection_find_indexes_with_opts (col, NULL);
  if(mongoc_cursor_error(c, &err))
    stop(err.message);

  return cursor2r(c, ptr_col);
}

SEXP R_mongo_collection_rename(SEXP ptr_col, SEXP db, SEXP name) {
  mongoc_collection_t *col = r2col(ptr_col);
  bson_error_t err;
  const char *new_db = NULL;
  if(db != R_NilValue)
    new_db = Rf_translateCharUTF8(Rf_asChar(db));

  if(!mongoc_collection_rename(col, new_db, Rf_translateCharUTF8(Rf_asChar(name)), false, &err))
    stop(err.message);
  return Rf_ScalarLogical(1);
}

SEXP R_mongo_collection_aggregate(SEXP ptr_col, SEXP ptr_pipeline, SEXP ptr_options, SEXP no_timeout) {
  mongoc_collection_t *col = r2col(ptr_col);
  bson_t *pipeline = r2bson(ptr_pipeline);
  bson_t *options = r2bson(ptr_options);

  mongoc_query_flags_t flags = MONGOC_QUERY_NONE;
  if(Rf_asLogical(no_timeout))
    flags += MONGOC_QUERY_NO_CURSOR_TIMEOUT;

  mongoc_cursor_t *c = mongoc_collection_aggregate (col, flags, pipeline, options, NULL);
  if(!c)
    stop("Error executing pipeline.");
  return cursor2r(c, ptr_col);
}

SEXP R_mongo_collection_command(SEXP ptr_col, SEXP ptr_cmd, SEXP no_timeout){
  mongoc_collection_t *col = r2col(ptr_col);
  bson_t *cmd = r2bson(ptr_cmd);

  mongoc_query_flags_t flags = MONGOC_QUERY_NONE;
  if(Rf_asLogical(no_timeout))
    flags += MONGOC_QUERY_NO_CURSOR_TIMEOUT;

  mongoc_cursor_t *c = mongoc_collection_command(col, flags, 0, 0, 0, cmd, NULL, NULL);
  if(!c)
    stop("Error executing command.");
  return cursor2r(c, ptr_col);
}

SEXP R_mongo_collection_command_simple(SEXP ptr_col, SEXP command){
  mongoc_collection_t *col = r2col(ptr_col);
  bson_t *cmd = r2bson(command);
  bson_t reply;
  bson_error_t err;
  if(!mongoc_collection_command_simple(col, cmd, NULL, &reply, &err))
    stop(err.message);

  SEXP out = PROTECT(bson2list(&reply));
  bson_destroy (&reply);
  UNPROTECT(1);
  return out;
}

SEXP R_ptr_get_prot(SEXP ptr_col){
  return R_ExternalPtrProtected(ptr_col);
}
