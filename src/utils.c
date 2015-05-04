#include <mongolite.h>

SEXP mkStringUTF8(const char* str){
  SEXP out = PROTECT(allocVector(STRSXP, 1));
  SET_STRING_ELT(out, 0, Rf_mkCharCE(str, CE_UTF8));
  UNPROTECT(1);
  return out;
}

SEXP mkRaw(const unsigned char *buf, int len){
  //create raw vector
  SEXP out = PROTECT(allocVector(RAWSXP, len));
  for (int i = 0; i < len; i++) {
    RAW(out)[i] = buf[i];
  }
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

SEXP bson2r(bson_t* b){
  SEXP ptr = PROTECT(R_MakeExternalPtr(b, R_NilValue, R_NilValue));
  R_RegisterCFinalizerEx(ptr, fin_bson, 1);
  setAttrib(ptr, R_ClassSymbol, mkString("bson"));
  UNPROTECT(1);
  return ptr;
}

SEXP cursor2r(mongoc_cursor_t* c){
  SEXP ptr = PROTECT(R_MakeExternalPtr(c, R_NilValue, R_NilValue));
  R_RegisterCFinalizerEx(ptr, fin_cursor, 1);
  setAttrib(ptr, R_ClassSymbol, mkString("mongo_cursor"));
  UNPROTECT(1);
  return ptr;
}

SEXP col2r(mongoc_collection_t *col){
  SEXP ptr = PROTECT(R_MakeExternalPtr(col, R_NilValue, R_NilValue));
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

void fin_mongo(SEXP ptr){
  #ifdef MONGOLITE_DEBUG
  MONGOC_MESSAGE ("destorying collection.");
  #endif
  if(!R_ExternalPtrAddr(ptr)) return;
  mongoc_collection_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}

void fin_bson(SEXP ptr){
  #ifdef MONGOLITE_DEBUG
  MONGOC_MESSAGE("destorying BSON.");
  #endif
  if(!R_ExternalPtrAddr(ptr)) return;
  bson_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}

void fin_cursor(SEXP ptr){
  #ifdef MONGOLITE_DEBUG
  MONGOC_MESSAGE("destorying cursor.");
  #endif

  if(!R_ExternalPtrAddr(ptr)) return;
  mongoc_cursor_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}

void fin_client(SEXP ptr){
  #ifdef MONGOLITE_DEBUG
  MONGOC_MESSAGE("destorying client.");
  #endif

  if(!R_ExternalPtrAddr(ptr)) return;
  mongoc_client_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}
