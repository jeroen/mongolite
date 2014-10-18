#include <Rinternals.h>
#include <bson.h>
#include <mongoc.h>
#include <utils.h>

SEXP R_json_to_bson(SEXP json);
void fin_bson(SEXP ptr);

SEXP R_json_to_bson(SEXP json){
  bson_t b;
  bson_error_t err;
  if(!bson_init_from_json(&b, translateCharUTF8(asChar(json)), -1, &err))
    error(err.message);

  //destroy on gc
  SEXP ptr = PROTECT(R_MakeExternalPtr(&b, R_NilValue, R_NilValue));
  R_RegisterCFinalizerEx(ptr, fin_bson, 1);
  setAttrib(ptr, R_ClassSymbol, mkString("bson"));
  UNPROTECT(1);

  //debug:
  //bson_t *doc = R_ExternalPtrAddr(ptr);
  //Rprintf(bson_as_json (doc, NULL));
  return ptr;
}

SEXP R_bson_to_json(SEXP ptr){
  bson_t *b = R_ExternalPtrAddr(ptr);
  if(!b)
    error("BSON document is null.");

  //this gives segfault
  //libbson seems to free up the bson object even when we do not call bson_destroy.
  return mkStringUTF8(bson_as_json (b, NULL));
}

void fin_bson(SEXP ptr){
  Rprintf("DEBUG: Destorying BSON.\n");
  if(!R_ExternalPtrAddr(ptr)) return;
  bson_destroy(R_ExternalPtrAddr(ptr));
  R_ClearExternalPtr(ptr);
}

