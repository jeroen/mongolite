#include <Rinternals.h>
#include <bson.h>
#include <mongoc.h>
#include <utils.h>

SEXP R_mongo_cursor_more (SEXP ptr){
  mongoc_cursor_t *c = r2cursor(ptr);
  return ScalarLogical(mongoc_cursor_more(c));
}

SEXP R_mongo_cursor_next (SEXP ptr){
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
