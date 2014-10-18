#include <Rinternals.h>
#include <bson.h>
#include <mongoc.h>
#include <utils.h>

SEXP R_json_to_bson(SEXP json);
void fin_bson(SEXP ptr);

SEXP R_json_to_bson(SEXP json){
  bson_t *b;
  bson_error_t err;

  b = bson_new_from_json ((uint8_t*)translateCharUTF8(asChar(json)), -1, &err);

  if(!b)
    error(err.message);

  //destroy on gc
  SEXP ptr = PROTECT(R_MakeExternalPtr(b, R_NilValue, R_NilValue));
  R_RegisterCFinalizerEx(ptr, fin_bson, 1);
  setAttrib(ptr, R_ClassSymbol, mkString("bson"));
  UNPROTECT(1);
  return ptr;
}

SEXP R_bson_to_json(SEXP ptr){
  bson_t *b = R_ExternalPtrAddr(ptr);
  if(!b)
    error("BSON document is null.");
  return mkStringUTF8(bson_as_json (b, NULL));
}

void fin_bson(SEXP ptr){
  Rprintf("DEBUG: Destorying BSON.\n");
  if(!R_ExternalPtrAddr(ptr)) return;
  bson_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}

