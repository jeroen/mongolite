#include <mongolite.h>
#include <stdio.h>

SEXP R_null_ptr(SEXP ptr){
  return ScalarLogical(!R_ExternalPtrAddr(ptr));
}

SEXP mkStringUTF8(const char* str){
  SEXP out = PROTECT(allocVector(STRSXP, 1));
  SET_STRING_ELT(out, 0, Rf_mkCharCE(str, CE_UTF8));
  UNPROTECT(1);
  return out;
}

SEXP bson_to_str(const bson_t * b){
  if(b == NULL)
    return ScalarString(NA_STRING);
  size_t jsonlength;
  char *str = bson_as_json(b, &jsonlength);
  if(str == NULL)
    return ScalarString(NA_STRING);
  SEXP out = ScalarString(mkCharLenCE(str, jsonlength, CE_UTF8));
  bson_free(str);
  return out;
}

SEXP mkRaw(const unsigned char *buf, int len){
  //create raw vector
  SEXP out = PROTECT(allocVector(RAWSXP, len));
  if(len)
    memcpy(RAW(out), buf, len);
  UNPROTECT(1);
  return out;
}

SEXP bson2list(bson_t *b){
  bson_iter_t iter1;
  bson_iter_t iter2;
  bson_iter_init(&iter1, b);
  bson_iter_init(&iter2, b);
  return ConvertObject(&iter1, &iter2);
}

bson_t* r2bson(SEXP ptr){
  bson_t *b = R_ExternalPtrAddr(ptr);
  if(!b)
    error("BSON object has been destroyed.");
  return b;
}

mongoc_collection_t* r2col(SEXP ptr){
  mongoc_collection_t * col = R_ExternalPtrAddr(ptr);
  if(!col)
    error("Collection has been destroyed.");
  return col;
}

mongoc_gridfs_t* r2gridfs(SEXP ptr){
  mongoc_gridfs_t* c = R_ExternalPtrAddr(ptr);
  if(!c)
    error("This grid has been destroyed.");
  return c;
}

mongoc_cursor_t* r2cursor(SEXP ptr){
  mongoc_cursor_t* c = R_ExternalPtrAddr(ptr);
  if(!c)
    error("Cursor has been destroyed.");
  return c;
}

mongoc_client_t* r2client(SEXP ptr){
  mongoc_client_t *client = R_ExternalPtrAddr(ptr);
  if(!client)
    error("Client has been destroyed.");
  return client;
}

static void fin_mongo(SEXP ptr){
#ifdef MONGOLITE_DEBUG
  MONGOC_MESSAGE ("destorying collection.");
#endif
  if(!R_ExternalPtrAddr(ptr)) return;
  mongoc_collection_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}

static void fin_bson(SEXP ptr){
#ifdef MONGOLITE_DEBUG
  MONGOC_MESSAGE("destorying BSON.");
#endif
  if(!R_ExternalPtrAddr(ptr)) return;
  bson_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}

static void fin_cursor(SEXP ptr){
#ifdef MONGOLITE_DEBUG
  MONGOC_MESSAGE("destorying cursor.");
#endif

  if(!R_ExternalPtrAddr(ptr)) return;
  mongoc_cursor_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}

static void fin_client(SEXP ptr){
#ifdef MONGOLITE_DEBUG
  MONGOC_MESSAGE("destorying client.");
#endif

  if(!R_ExternalPtrAddr(ptr)) return;
  mongoc_client_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}

static void fin_gridfs(SEXP ptr){
#ifdef MONGOLITE_DEBUG
  MONGOC_MESSAGE("destorying gridfs.");
#endif

  if(!R_ExternalPtrAddr(ptr)) return;
  mongoc_gridfs_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}

SEXP bson2r(bson_t* b){
  SEXP ptr = PROTECT(R_MakeExternalPtr(b, R_NilValue, R_NilValue));
  R_RegisterCFinalizerEx(ptr, fin_bson, 1);
  setAttrib(ptr, R_ClassSymbol, mkString("bson"));
  UNPROTECT(1);
  return ptr;
}

SEXP cursor2r(mongoc_cursor_t* c, SEXP prot){
  SEXP ptr = PROTECT(R_MakeExternalPtr(c, R_NilValue, prot));
  R_RegisterCFinalizerEx(ptr, fin_cursor, 1);
  setAttrib(ptr, R_ClassSymbol, mkString("mongo_cursor"));
  UNPROTECT(1);
  return ptr;
}

SEXP gridfs2r(mongoc_gridfs_t *fs, SEXP prot){
  SEXP ptr = PROTECT(R_MakeExternalPtr(fs, R_NilValue, prot));
  R_RegisterCFinalizerEx(ptr, fin_gridfs, 1);
  setAttrib(ptr, R_ClassSymbol, mkString("mongo_gridfs"));
  UNPROTECT(1);
  return ptr;
}

SEXP col2r(mongoc_collection_t *col, SEXP prot){
  SEXP ptr = PROTECT(R_MakeExternalPtr(col, R_NilValue, prot));
  R_RegisterCFinalizerEx(ptr, fin_mongo, 1);
  setAttrib(ptr, R_ClassSymbol, mkString("mongo_collection"));
  UNPROTECT(1);
  return ptr;
}

SEXP client2r(mongoc_client_t *client){
  SEXP ptr = PROTECT(R_MakeExternalPtr(client, R_NilValue, R_NilValue));
  R_RegisterCFinalizerEx(ptr, fin_client, 1);
  setAttrib(ptr, R_ClassSymbol, mkString("mongo_client"));
  UNPROTECT(1);
  return ptr;
}
