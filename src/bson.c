#include <Rinternals.h>
#include <bson.h>
#include <mongoc.h>

SEXP R_json_to_bson(SEXP json);
void r_bson_clean(SEXP ptr);

SEXP R_json_to_bson(SEXP json){
  bson_t doc;
  bson_error_t err;
  if(!bson_init_from_json(&doc, translateCharUTF8(asChar(json)), -1, &err))
    error(err.message);

  //destroy on gc
  SEXP ptr = PROTECT(R_MakeExternalPtr(&doc, R_NilValue, R_NilValue));
  R_RegisterCFinalizerEx(ptr, r_bson_clean, 1);
  setAttrib(ptr, R_ClassSymbol, mkString("bson"));
  UNPROTECT(1);
  return ptr;
}

void r_bson_clean(SEXP ptr){
  if(!R_ExternalPtrAddr(ptr)) return;
  bson_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}

