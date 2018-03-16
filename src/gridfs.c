#include <mongolite.h>

SEXP R_mongo_gridfs_new(SEXP ptr_client, SEXP prefix, SEXP db) {
  mongoc_client_t *client = r2client(ptr_client);
  bson_error_t err;
  mongoc_gridfs_t * fs = mongoc_client_get_gridfs(client, translateCharUTF8(asChar(db)), translateCharUTF8(asChar(prefix)), &err);
  if(fs == NULL)
    stop(err.message);
  SEXP out = PROTECT(gridfs2r(fs, ptr_client));
  setAttrib(out, install("client"), ptr_client);
  UNPROTECT(1);
  return out;
}

SEXP R_mongo_gridfs_drop (SEXP ptr_fs){
  mongoc_gridfs_t *fs = r2gridfs(ptr_fs);
  bson_error_t err;
  if(!mongoc_gridfs_drop(fs, &err))
    stop(err.message);
  return ScalarLogical(1);
}

SEXP R_mongo_gridfs_list(SEXP ptr_fs, SEXP ptr_filter, SEXP ptr_opts){
  mongoc_gridfs_t *fs = r2gridfs(ptr_fs);
  bson_t *filter = r2bson(ptr_filter);
  bson_t *opts = r2bson(ptr_opts);
  mongoc_gridfs_file_list_t * list = mongoc_gridfs_find_with_opts (fs, filter, opts);

  /* iterate through results and store in linked list */
  SEXP ret = R_NilValue;
  mongoc_gridfs_file_t * file;
  while ((file = mongoc_gridfs_file_list_next (list))) {
    const char * name = mongoc_gridfs_file_get_filename (file);
    ret = PROTECT(Rf_cons(Rf_mkString(name), ret));
    mongoc_gridfs_file_destroy (file);
  }
  mongoc_gridfs_file_list_destroy (list);
  UNPROTECT(Rf_length(ret));
  return ret;
}

SEXP R_mongo_gridfs_read(SEXP ptr_fs, SEXP name){
  mongoc_gridfs_t *fs = r2gridfs(ptr_fs);
  bson_error_t err;
  mongoc_gridfs_file_t * file = mongoc_gridfs_find_one_by_filename (fs, CHAR(asChar(name)), &err);
  if(file == NULL)
    stop(err.message);

  ssize_t size = mongoc_gridfs_file_get_length(file);
  mongoc_stream_t * stream = mongoc_stream_gridfs_new (file);
  if(!stream)
    stop("Failed to create mongoc_stream_gridfs_new");

  SEXP out = Rf_allocVector(RAWSXP, size);
  if(mongoc_stream_read (stream, RAW(out), size, size, 0) < size)
    stop("Failed to read entire steam");

  mongoc_stream_destroy (stream);
  mongoc_gridfs_file_destroy (file);
  return out;
}

