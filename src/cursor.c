#include <mongolite.h>

SEXP R_mongo_cursor_more (SEXP ptr){
  mongoc_cursor_t *c = r2cursor(ptr);
  return ScalarLogical(mongoc_cursor_more(c));
}

SEXP R_mongo_cursor_next_bson (SEXP ptr){
  mongoc_cursor_t *c = r2cursor(ptr);
  const bson_t *b = NULL;
  if(!mongoc_cursor_next(c, &b)){
    bson_error_t err;
    if(mongoc_cursor_error (c, &err))
      stop(err.message);
    else
      return R_NilValue;
  }
  return bson2r((bson_t*) b);
}

SEXP R_mongo_cursor_next_page(SEXP ptr, SEXP size){
  mongoc_cursor_t *c = r2cursor(ptr);
  int n = asInteger(size);
  const bson_t *b = NULL;
  SEXP list = PROTECT(allocVector(VECSXP, n));
  int total = 0;
  for(int i = 0; i < n && mongoc_cursor_next(c, &b); i++){
    SET_VECTOR_ELT(list, i, bson2list((bson_t*) b));
    total++;
  }

  //found a full page
  if(total == n){
    UNPROTECT(1);
    return list;
  }

  //not a full page
  SEXP shortlist = PROTECT(allocVector(VECSXP, total));
  for(int i = 0; i < total; i++){
    SET_VECTOR_ELT(shortlist, i, VECTOR_ELT(list, i));
  }
  UNPROTECT(2);

  //also check for errors
  bson_error_t err;
  if(mongoc_cursor_error (c, &err))
    stop(err.message);

  return shortlist;
}
