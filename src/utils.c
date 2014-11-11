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
    error("BSON document is null.");
  return b;
}

mongoc_collection_t* r2col(SEXP ptr){
  mongoc_collection_t * col = R_ExternalPtrAddr(ptr);
  if(!col)
    error("Collection is null.");
  return col;
}

SEXP bson2r(bson_t* b){
  SEXP ptr = PROTECT(R_MakeExternalPtr(b, R_NilValue, R_NilValue));
  R_RegisterCFinalizerEx(ptr, fin_bson, 1);
  setAttrib(ptr, R_ClassSymbol, mkString("bson"));
  UNPROTECT(1);
  return ptr;
}

void fin_bson(SEXP ptr){
  Rprintf("DEBUG: Destorying BSON.\n");
  if(!R_ExternalPtrAddr(ptr)) return;
  bson_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}


