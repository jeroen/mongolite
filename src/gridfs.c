#include <mongolite.h>

SEXP make_string(const char * x){
  return ScalarString(x ? Rf_mkCharCE(x, CE_UTF8) : NA_STRING);
}

const char * get_string(SEXP x){
  if(!Rf_isString(x) || Rf_length(x) != 1)
    stop("Value is not a string of length 1");
  return translateCharUTF8(STRING_ELT(x, 0));
}

SEXP get_id_and_destroy(mongoc_gridfs_file_t * file){
  bson_t val;
  bson_init (&val);
  BSON_APPEND_VALUE(&val, "id", mongoc_gridfs_file_get_id(file));
  mongoc_gridfs_file_destroy (file);
  return bson2list(&val);
}

SEXP save_file_and_get_id(mongoc_gridfs_file_t * file){
  if(!mongoc_gridfs_file_save (file)){
    bson_error_t err;
    mongoc_gridfs_file_error(file, &err);
    stop(err.message);
  }
  return get_id_and_destroy(file);
}

SEXP R_mongo_gridfs_new(SEXP ptr_client, SEXP prefix, SEXP db) {
  mongoc_client_t *client = r2client(ptr_client);
  bson_error_t err;
  mongoc_gridfs_t * fs = mongoc_client_get_gridfs(client, get_string(db), get_string(prefix), &err);
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

SEXP R_mongo_gridfs_find(SEXP ptr_fs, SEXP ptr_filter, SEXP ptr_opts){
  mongoc_gridfs_t *fs = r2gridfs(ptr_fs);
  bson_t *filter = r2bson(ptr_filter);
  bson_t *opts = r2bson(ptr_opts);
  mongoc_gridfs_file_list_t * list = mongoc_gridfs_find_with_opts (fs, filter, opts);

  /* iterate through results and store in linked list */
  /* TODO: only protect head of lists */
  SEXP names = R_NilValue;
  SEXP sizes = R_NilValue;
  SEXP dates = R_NilValue;
  SEXP ids = R_NilValue;
  SEXP content_type = R_NilValue;
  mongoc_gridfs_file_t * file;
  while ((file = mongoc_gridfs_file_list_next (list))) {
    names = PROTECT(Rf_cons(make_string(mongoc_gridfs_file_get_filename (file)), names));
    sizes = PROTECT(Rf_cons(Rf_ScalarReal(mongoc_gridfs_file_get_length (file)), sizes));
    dates = PROTECT(Rf_cons(Rf_ScalarReal(mongoc_gridfs_file_get_upload_date (file)), dates));
    content_type = PROTECT(Rf_cons(make_string(mongoc_gridfs_file_get_content_type(file)), content_type));
    ids = PROTECT(Rf_cons(get_id_and_destroy(file), ids));
  }
  mongoc_gridfs_file_list_destroy (list);
  UNPROTECT(Rf_length(names) * 5);
  return Rf_list5(ids, names, sizes, dates, content_type);
}

SEXP R_mongo_gridfs_upload(SEXP ptr_fs, SEXP name, SEXP path, SEXP content_type, SEXP meta_ptr){
  mongoc_gridfs_t *fs = r2gridfs(ptr_fs);
  mongoc_stream_t * stream = mongoc_stream_file_new_for_path(CHAR(STRING_ELT(path, 0)), O_RDONLY, 0);
  if(stream == NULL)
    stop("Failure at mongoc_stream_file_new_for_path()");

  mongoc_gridfs_file_opt_t opt = {0};
  opt.filename = get_string(name);
  mongoc_gridfs_file_t * file = mongoc_gridfs_create_file_from_stream (fs, stream, &opt);
  if(file == NULL)
    stop("Failure at mongoc_gridfs_create_file_from_stream()");
  if(Rf_length(content_type) && STRING_ELT(content_type, 0) != NA_STRING)
    mongoc_gridfs_file_set_content_type(file, CHAR(STRING_ELT(content_type, 0)));
  if(Rf_length(meta_ptr))
    mongoc_gridfs_file_set_metadata(file, r2bson(meta_ptr));
  return save_file_and_get_id(file);
}

SEXP R_mongo_gridfs_write(SEXP ptr_fs, SEXP name, SEXP data, SEXP content_type, SEXP meta_ptr){
  mongoc_gridfs_t *fs = r2gridfs(ptr_fs);
  mongoc_gridfs_file_opt_t opt = {0};
  opt.filename = get_string(name);
  mongoc_gridfs_file_t * file = mongoc_gridfs_create_file(fs, &opt);
  if(file == NULL)
    stop("Failure at mongoc_gridfs_create_file()");

  mongoc_iovec_t iov = {0};
  iov.iov_len = Rf_length(data);
  iov.iov_base = RAW(data);
  if(mongoc_gridfs_file_writev(file, &iov, 1, 0) < iov.iov_len)
    stop("Failure at mongoc_gridfs_file_writev");
  if(Rf_length(content_type) && STRING_ELT(content_type, 0) != NA_STRING)
    mongoc_gridfs_file_set_content_type(file, CHAR(STRING_ELT(content_type, 0)));
  if(Rf_length(meta_ptr))
    mongoc_gridfs_file_set_metadata(file, r2bson(meta_ptr));
  return save_file_and_get_id(file);
}

SEXP R_mongo_gridfs_read(SEXP ptr_fs, SEXP name){
  mongoc_gridfs_t *fs = r2gridfs(ptr_fs);
  bson_error_t err;
  mongoc_gridfs_file_t * file = mongoc_gridfs_find_one_by_filename (fs, get_string(name), &err);
  if(file == NULL)
    stop(err.message);

  ssize_t size = mongoc_gridfs_file_get_length(file);
  mongoc_stream_t * stream = mongoc_stream_gridfs_new (file);
  if(!stream)
    stop("Failed to create mongoc_stream_gridfs_new");

  SEXP out = Rf_allocVector(RAWSXP, size);
  if(mongoc_stream_read (stream, RAW(out), size, -1, 0) < size)
    stop("Failed to read entire steam");

  mongoc_stream_destroy (stream);
  mongoc_gridfs_file_destroy(file);
  return out;
}

SEXP R_mongo_gridfs_download(SEXP ptr_fs, SEXP name, SEXP path){
  mongoc_gridfs_t *fs = r2gridfs(ptr_fs);
  bson_error_t err;
  mongoc_gridfs_file_t * file = mongoc_gridfs_find_one_by_filename (fs, get_string(name), &err);
  if(file == NULL)
    stop(err.message);

  mongoc_stream_t * stream = mongoc_stream_gridfs_new (file);
  if(!stream)
    stop("Failed to create mongoc_stream_gridfs_new");

  char buf[4096];
  FILE * fp = fopen(get_string(path), "wb");
  if(!fp)
    stop("Failed to open file %s", get_string(path));

  for(;;) {
    int nbytes = mongoc_stream_read(stream, buf, 4096, -1, 0);
    if(nbytes == 0)
      break;
    if(nbytes < 0)
      stop("Error in mongoc_stream_readv()");
    if (fwrite (buf, 1, nbytes, fp) != nbytes)
      stop("Failed to write to file");
  }
  fclose(fp);
  mongoc_stream_destroy (stream);
  mongoc_gridfs_file_destroy(file);
  return path;
}

SEXP R_mongo_gridfs_remove(SEXP ptr_fs, SEXP name){
  mongoc_gridfs_t *fs = r2gridfs(ptr_fs);
  bson_error_t err;
  mongoc_gridfs_file_t * file = mongoc_gridfs_find_one_by_filename (fs, get_string(name), &err);
  if(file == NULL)
    stop(err.message);
  if(!mongoc_gridfs_file_remove(file, &err))
    stop(err.message);
  return get_id_and_destroy(file);
}
