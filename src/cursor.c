#include <Rinternals.h>
#include <bson.h>
#include <mongoc.h>
#include <utils.h>

SEXP R_mongo_cursor_more (SEXP ptr){
  mongoc_cursor_t *c = r2cursor(ptr);
  return ScalarLogical(mongoc_cursor_more(c));
}

SEXP R_mongo_cursor_next_bson (SEXP ptr){
  mongoc_cursor_t *c = r2cursor(ptr);
  const bson_t *b = NULL;
  if(!mongoc_cursor_next(c, &b)){
    /*
    # Bug in mongoc? See https://github.com/mongodb/mongo-c-driver/issues/66
    bson_error_t err;
    mongoc_cursor_error (c, &err);
    error(err.message);
    */
    return R_NilValue;
  }
  return bson2r((bson_t*) b);
}

SEXP R_mongo_cursor_next_page(SEXP ptr, SEXP size){
  mongoc_cursor_t *c = r2cursor(ptr);
  int n = asInteger(size);
  const bson_t *b = NULL;
  bson_iter_t iter1;
  bson_iter_t iter2;
  SEXP list = PROTECT(allocVector(VECSXP, n));
  for(int i = 0; i < n && mongoc_cursor_next(c, &b); i++){
    bson_iter_init(&iter1, b);
    bson_iter_init(&iter2, b);
    SET_VECTOR_ELT(list, i, ConvertObject(&iter1, &iter2));
  }
  UNPROTECT(1);
  return list;
}
