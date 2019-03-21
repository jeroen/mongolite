#include <mongolite.h>

static SEXP make_string(const char * x){
  return Rf_ScalarString(x ? Rf_mkCharCE(x, CE_UTF8) : NA_STRING);
}

static SEXP make_date(double val){
  SEXP out = PROTECT(Rf_ScalarReal(val / 1000));
  SEXP cls = PROTECT(Rf_allocVector(STRSXP, 2));
  SET_STRING_ELT(cls, 0, Rf_mkChar("POSIXct"));
  SET_STRING_ELT(cls, 1, Rf_mkChar("POSIXt"));
  Rf_setAttrib(out, R_ClassSymbol, cls);
  UNPROTECT(2);
  return out;
}

static const char * get_string(SEXP x){
  if(!Rf_isString(x) || Rf_length(x) != 1)
    stop("Value is not a string of length 1");
  return Rf_translateCharUTF8(STRING_ELT(x, 0));
}

static SEXP append_tail(SEXP el, SEXP tail){
  return SETCDR(tail, Rf_cons(el, R_NilValue));
}

static SEXP get_file_id(mongoc_gridfs_file_t * file){
  bson_t val;
  bson_init (&val);
  BSON_APPEND_VALUE(&val, "id", mongoc_gridfs_file_get_id(file));
  SEXP lst = bson2list(&val);
  return Rf_length(lst) ? VECTOR_ELT(bson2list(&val), 0) : R_NilValue;
}

static SEXP create_outlist(mongoc_gridfs_file_t * file){
  SEXP out = PROTECT(Rf_allocVector(VECSXP, 6));
  SET_VECTOR_ELT(out, 0, get_file_id(file));
  SET_VECTOR_ELT(out, 1, make_string(mongoc_gridfs_file_get_filename(file)));
  SET_VECTOR_ELT(out, 2, Rf_ScalarReal(mongoc_gridfs_file_get_length (file)));
  SET_VECTOR_ELT(out, 3, make_date(mongoc_gridfs_file_get_upload_date (file)));
  SET_VECTOR_ELT(out, 4, make_string(mongoc_gridfs_file_get_content_type(file)));
  SET_VECTOR_ELT(out, 5, bson_to_str(mongoc_gridfs_file_get_metadata(file)));
  SEXP nms = PROTECT(Rf_allocVector(STRSXP, 6));
  Rf_setAttrib(out, R_NamesSymbol, nms);
  SET_STRING_ELT(nms, 0, Rf_mkChar("id"));
  SET_STRING_ELT(nms, 1, Rf_mkChar("name"));
  SET_STRING_ELT(nms, 2, Rf_mkChar("size"));
  SET_STRING_ELT(nms, 3, Rf_mkChar("date"));
  SET_STRING_ELT(nms, 4, Rf_mkChar("type"));
  SET_STRING_ELT(nms, 5, Rf_mkChar("metadata"));
  UNPROTECT(2);
  return out;
}

/* find either by string or by query */
static mongoc_gridfs_file_t * find_single_file(SEXP ptr_fs, SEXP name){
  mongoc_gridfs_t *fs = r2gridfs(ptr_fs);
  bson_error_t err;
  mongoc_gridfs_file_t * file = Rf_isString(name) ?
  mongoc_gridfs_find_one_by_filename (fs, get_string(name), &err) :
    mongoc_gridfs_find_one_with_opts(fs, r2bson(name), NULL, &err);
  if(file == NULL)
    stop("File not found. %s", err.message);
  return file;
}

SEXP R_mongo_gridfs_new(SEXP ptr_client, SEXP prefix, SEXP db) {
  mongoc_client_t *client = r2client(ptr_client);
  bson_error_t err;
  mongoc_gridfs_t * fs = mongoc_client_get_gridfs(client, get_string(db), get_string(prefix), &err);
  if(fs == NULL)
    stop(err.message);
  return gridfs2r(fs, ptr_client);
}

SEXP R_mongo_gridfs_drop (SEXP ptr_fs){
  mongoc_gridfs_t *fs = r2gridfs(ptr_fs);
  bson_error_t err;
  int res = mongoc_gridfs_drop(fs, &err);
  if(!res && err.code != 26)
    stop(err.message);
  return Rf_ScalarLogical(res);
}

SEXP R_mongo_gridfs_find(SEXP ptr_fs, SEXP ptr_filter, SEXP ptr_opts){
  mongoc_gridfs_t *fs = r2gridfs(ptr_fs);
  bson_t *filter = r2bson(ptr_filter);
  bson_t *opts = r2bson(ptr_opts);
  mongoc_gridfs_file_list_t * list = mongoc_gridfs_find_with_opts (fs, filter, opts);

  /* Protect HEAD sentinel nodes for each list */
  mongoc_gridfs_file_t * file;
  SEXP head = PROTECT(Rf_list1(R_NilValue));
  SEXP tail = head;
  while ((file = mongoc_gridfs_file_list_next (list))) {
    tail = append_tail(create_outlist(file), tail);
    mongoc_gridfs_file_destroy (file);
  }
  mongoc_gridfs_file_list_destroy (list);
  UNPROTECT(1);
  return CDR(head); // CDR() drops sentinel node
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
  if(!mongoc_gridfs_file_save (file)){
    bson_error_t err;
    mongoc_gridfs_file_error(file, &err);
    mongoc_gridfs_file_destroy (file);
    stop(err.message);
  }
  SEXP val = PROTECT(create_outlist(file));
  mongoc_gridfs_file_destroy (file);
  UNPROTECT(1);
  return val;
}

SEXP R_mongo_gridfs_download(SEXP ptr_fs, SEXP name, SEXP path){
  mongoc_gridfs_file_t *file = find_single_file(ptr_fs, name);
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
  SEXP val = PROTECT(create_outlist(file));
  mongoc_gridfs_file_destroy(file);
  UNPROTECT(1);
  return val;
}

SEXP R_mongo_gridfs_remove(SEXP ptr_fs, SEXP name){
  bson_error_t err;
  mongoc_gridfs_file_t *file = find_single_file(ptr_fs, name);
  if(!mongoc_gridfs_file_remove(file, &err))
    stop(err.message);
  SEXP val = PROTECT(create_outlist(file));
  mongoc_gridfs_file_destroy (file);
  UNPROTECT(1);
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
  filestream * filestr = R_ExternalPtrAddr(ptr);
  if(filestr->stream)
    mongoc_stream_destroy(filestr->stream);
  if(filestr->file)
    mongoc_gridfs_file_destroy(filestr->file);
  free(filestr);
  R_SetExternalPtrProtected(ptr, R_NilValue);
  R_ClearExternalPtr(ptr);
}

static filestream * get_stream_ptr(SEXP ptr){
  filestream * filestr = R_ExternalPtrAddr(ptr);
  if(!filestr)
    Rf_error("stream has been destroyed.");
  return filestr;
}

static SEXP R_make_stream_ptr(mongoc_gridfs_file_t * file, SEXP ptr_fs){
  mongoc_stream_t *stream = mongoc_stream_gridfs_new (file);
  if(!stream){
    mongoc_gridfs_file_destroy(file);
    stop("Failed to create mongoc_stream_gridfs_new");
  }
  double size = mongoc_gridfs_file_get_length(file);
  if(size < 0)
    size = NA_REAL;
  filestream * filestr = malloc(sizeof (filestream));
  filestr->file = file;
  filestr->stream = stream;
  SEXP ptr = PROTECT(R_MakeExternalPtr(filestr, R_NilValue, ptr_fs));
  R_RegisterCFinalizerEx(ptr, fin_filestream, 1);
  Rf_setAttrib(ptr, R_ClassSymbol, Rf_mkString("filestream"));
  Rf_setAttrib(ptr, Rf_install("size"), PROTECT(Rf_ScalarReal(size)));
  UNPROTECT(2);
  return ptr;
}

SEXP R_new_read_stream(SEXP ptr_fs, SEXP name){
  mongoc_gridfs_file_t * file = find_single_file(ptr_fs, name);
  return R_make_stream_ptr(file, ptr_fs);
}

SEXP R_new_write_stream(SEXP ptr_fs, SEXP name, SEXP content_type, SEXP meta_ptr){
  mongoc_gridfs_t *fs = r2gridfs(ptr_fs);
  mongoc_gridfs_file_opt_t opt = {0};
  opt.filename = get_string(name);
  mongoc_gridfs_file_t * file = mongoc_gridfs_create_file (fs, &opt);
  if(file == NULL)
    stop("Failure at mongoc_gridfs_create_file()");
  if(Rf_length(content_type) && STRING_ELT(content_type, 0) != NA_STRING)
    mongoc_gridfs_file_set_content_type(file, CHAR(STRING_ELT(content_type, 0)));
  if(Rf_length(meta_ptr))
    mongoc_gridfs_file_set_metadata(file, r2bson(meta_ptr));
  return R_make_stream_ptr(file, ptr_fs);
}

SEXP R_stream_read_chunk(SEXP ptr, SEXP n){
  double bufsize = Rf_asReal(n);
  filestream * filestr = get_stream_ptr(ptr);
  SEXP buf = PROTECT(Rf_allocVector(RAWSXP, bufsize));
  ssize_t len = mongoc_stream_read (filestr->stream, RAW(buf), bufsize, -1, 0);
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

SEXP R_stream_write_chunk(SEXP ptr, SEXP buf){
  ssize_t len = 0;
  filestream * filestr = get_stream_ptr(ptr);
  if(Rf_length(buf)){
    len = mongoc_stream_write (filestr->stream, RAW(buf), Rf_length(buf), 0);
    if(len < 0)
      Rf_error("Error writing to stream");
    if(len < Rf_length(buf))
      Rf_error("Incomplete stream write");
  } else {
    if(!mongoc_gridfs_file_save (filestr->file)){
      bson_error_t err;
      mongoc_gridfs_file_error(filestr->file, &err);
      stop(err.message);
    }
  }
  return Rf_ScalarInteger(len);
}

SEXP R_stream_close(SEXP ptr){
  SEXP val = PROTECT(create_outlist(get_stream_ptr(ptr)->file));
  fin_filestream(ptr);
  UNPROTECT(1);
  return val;
}
