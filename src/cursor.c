#include <Rinternals.h>
#include <bson.h>
#include <mongoc.h>
#include <utils.h>

SEXP R_mongoc_cursor_next (SEXP ptr){
  mongoc_cursor_t *c = r2cursor(ptr);
  if(!mongoc_cursor_more(c)){
    return R_NilValue;
  }
  const bson_t *b = NULL;
  bson_error_t err;
  if(!mongoc_cursor_next(c, &b)){
    mongoc_cursor_error (c, &err);
    error(err.message);
  }
  return bson2r((bson_t*) b);
}
