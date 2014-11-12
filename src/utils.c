#include <utils.h>
#include <bson.h>

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

SEXP R_mongo_init() {
  mongoc_init();
  return R_NilValue;
}

SEXP R_mongo_cleanup() {
  mongoc_cleanup();
  return R_NilValue;
}

void fin_mongo(SEXP ptr){
  Rprintf("DEBUG: Destorying collection.\n");
  if(!R_ExternalPtrAddr(ptr)) return;
  mongoc_collection_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}

void fin_bson(SEXP ptr){
  Rprintf("DEBUG: Destorying BSON.\n");
  if(!R_ExternalPtrAddr(ptr)) return;
  bson_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}

void fin_cursor(SEXP ptr){
  Rprintf("DEBUG: Destorying cursor.\n");
  if(!R_ExternalPtrAddr(ptr)) return;
  mongoc_cursor_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}

void fin_client(SEXP ptr){
  Rprintf("DEBUG: Destorying client.\n");
  if(!R_ExternalPtrAddr(ptr)) return;
  mongoc_client_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}
