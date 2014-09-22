#include <Rinternals.h>
#include <mongoc.h>

SEXP R_mongo_init() {
    mongoc_init();
    return R_NilValue;
}