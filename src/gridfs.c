#include <mongolite.h>

SEXP make_string(const char * x){
  return ScalarString(x ? Rf_mkCharCE(x, CE_UTF8) : NA_STRING);
}

const char * get_string(SEXP x){
  if(!Rf_isString(x) || Rf_length(x) != 1)
    stop("Value is not a string of length 1");
  return translateCharUTF8(STRING_ELT(x, 0));
}

SEXP append_tail(SEXP el, SEXP tail){
  return SETCDR(tail, Rf_cons(el, R_NilValue));
}

SEXP get_file_id(mongoc_gridfs_file_t * file){
  bson_t val;
  bson_init (&val);
  BSON_APPEND_VALUE(&val, "id", mongoc_gridfs_file_get_id(file));
  SEXP lst = bson2list(&val);
  return Rf_length(lst) ? VECTOR_ELT(bson2list(&val), 0) : R_NilValue;
}

SEXP save_file_and_get_id(mongoc_gridfs_file_t * file){
  if(!mongoc_gridfs_file_save (file)){
    bson_error_t err;
    mongoc_gridfs_file_error(file, &err);
    stop(err.message);
  }
  SEXP val = get_file_id(file);
  mongoc_gridfs_file_destroy (file);
  return val;
}

SEXP create_outlist(mongoc_gridfs_file_t * file, SEXP data){
  PROTECT(data);
  SEXP filename = PROTECT(make_string(mongoc_gridfs_file_get_filename(file)));
  SEXP content_type = PROTECT(make_string(mongoc_gridfs_file_get_content_type(file)));
  SEXP metadata = PROTECT(bson_to_str(mongoc_gridfs_file_get_metadata(file)));
  SEXP id = PROTECT(get_file_id(file));
  SEXP out = Rf_list5(id, filename, content_type, metadata, data);
  UNPROTECT(5);
  return out;
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

  /* Protect HEAD sentinel nodes for each list */
  SEXP a,b,c,d,e;
  SEXP ids = a = PROTECT(Rf_list1(R_NilValue));
  SEXP names = b = PROTECT(Rf_list1(R_NilValue));
  SEXP sizes = c = PROTECT(Rf_list1(R_NilValue));
  SEXP dates = d = PROTECT(Rf_list1(R_NilValue));
  SEXP content_type = e = PROTECT(Rf_list1(R_NilValue));
  mongoc_gridfs_file_t * file;
  while ((file = mongoc_gridfs_file_list_next (list))) {
    names = append_tail(make_string(mongoc_gridfs_file_get_filename (file)), names);
    sizes = append_tail(Rf_ScalarReal(mongoc_gridfs_file_get_length (file)), sizes);
    dates = append_tail(Rf_ScalarReal(mongoc_gridfs_file_get_upload_date (file)), dates);
    content_type = append_tail(make_string(mongoc_gridfs_file_get_content_type(file)), content_type);
    ids = append_tail(get_file_id(file), ids);
    mongoc_gridfs_file_destroy (file);
  }
  mongoc_gridfs_file_list_destroy (list);
  SEXP out = Rf_list5(CDR(a), CDR(b), CDR(c), CDR(d), CDR(e)); // CDR() drops sentinel node
  UNPROTECT(5);
  return out;
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
    stop("File not found. %s", err.message);

  ssize_t size = mongoc_gridfs_file_get_length(file);
  mongoc_stream_t * stream = mongoc_stream_gridfs_new (file);
  if(!stream)
    stop("Failed to create mongoc_stream_gridfs_new");

  SEXP buf = Rf_allocVector(RAWSXP, size);
  if(mongoc_stream_read (stream, RAW(buf), size, -1, 0) < size)
    stop("Failed to read entire steam");
  mongoc_stream_destroy (stream);
  SEXP val = create_outlist(file, buf);
  mongoc_gridfs_file_destroy (file);
  return val;
}

SEXP R_mongo_gridfs_download(SEXP ptr_fs, SEXP name, SEXP path){
  mongoc_gridfs_t *fs = r2gridfs(ptr_fs);
  bson_error_t err;
  mongoc_gridfs_file_t * file = mongoc_gridfs_find_one_by_filename (fs, get_string(name), &err);
  if(file == NULL)
    stop("File not found. %s", err.message);

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
    if(nbytes < 0){
      fclose(fp);
      stop("Error in mongoc_stream_readv()");
    }
    if (fwrite (buf, 1, nbytes, fp) != nbytes){
      fclose(fp);
      stop("Failed to write to file");
    }
  }
  fclose(fp);
  mongoc_stream_destroy (stream);
  SEXP val = create_outlist(file, path);
  mongoc_gridfs_file_destroy (file);
  return val;
}

SEXP R_mongo_gridfs_remove(SEXP ptr_fs, SEXP name){
  mongoc_gridfs_t *fs = r2gridfs(ptr_fs);
  bson_error_t err;
  mongoc_gridfs_file_t * file = mongoc_gridfs_find_one_by_filename (fs, get_string(name), &err);
  if(file == NULL)
    stop("File not found. %s", err.message);
  if(!mongoc_gridfs_file_remove(file, &err))
    stop(err.message);
  SEXP val = get_file_id(file);
  mongoc_gridfs_file_destroy (file);
  return val;
}

/* Connection Streaming API */

typedef struct {
  mongoc_stream_t * stream;
  mongoc_gridfs_file_t * file;
} filestream;

static void fin_filestream(SEXP ptr){
#ifdef MONGOLITE_DEBUG
  MONGOC_MESSAGE ("destorying stream.");
#endif
  if(!R_ExternalPtrAddr(ptr))
    return;
  filestream * filestream = R_ExternalPtrAddr(ptr);
  if(filestream->stream)
    mongoc_stream_destroy(filestream->stream);
  if(filestream->file)
    mongoc_gridfs_file_destroy(filestream->file);
  free(filestream);
  R_ClearExternalPtr(ptr);
}

static filestream * get_stream_ptr(SEXP ptr){
  filestream * filestream = R_ExternalPtrAddr(ptr);
  if(!filestream)
    error("stream has been destroyed.");
  return filestream;
}

SEXP R_new_stream_ptr(SEXP ptr_fs, SEXP name){
  mongoc_gridfs_t *fs = r2gridfs(ptr_fs);
  bson_error_t err;
  mongoc_gridfs_file_t * file = mongoc_gridfs_find_one_by_filename (fs, get_string(name), &err);
  if(file == NULL)
    stop("File not found. %s", err.message);

  ssize_t size = mongoc_gridfs_file_get_length(file);
  if(size < 0)
    stop("Invalid filesize: %d\n", size);
  mongoc_stream_t *stream = mongoc_stream_gridfs_new (file);
  if(!stream){
    mongoc_gridfs_file_destroy(file);
    stop("Failed to create mongoc_stream_gridfs_new");
  }
  filestream * filestream = malloc(sizeof filestream);
  filestream->file = file;
  filestream->stream = stream;
  SEXP ptr = PROTECT(R_MakeExternalPtr(filestream, R_NilValue, R_NilValue));
  R_RegisterCFinalizerEx(ptr, fin_filestream, 1);
  setAttrib(ptr, R_ClassSymbol, mkString("filestream"));
  setAttrib(ptr, install("size"), ScalarReal(size));
  UNPROTECT(1);
  return ptr;
}

SEXP R_read_stream_ptr(SEXP ptr, SEXP n){
  double bufsize = Rf_asReal(n);
  filestream * filestream = get_stream_ptr(ptr);
  SEXP buf = PROTECT(Rf_allocVector(RAWSXP, bufsize));
  ssize_t len = mongoc_stream_read (filestream->stream, RAW(buf), bufsize, -1, 0);
  if(len < 0)
    Rf_error("Error reading from stream");
  if(len < bufsize){
    SEXP orig = buf;
    buf = Rf_allocVector(RAWSXP, len);
    memcpy(RAW(buf), RAW(orig), len);
  }
  UNPROTECT(1);
  return buf;
}

SEXP R_close_stream_ptr(SEXP ptr){
  SEXP val = create_outlist(get_stream_ptr(ptr)->file, R_NilValue);
  fin_filestream(ptr);
  return val;
}
