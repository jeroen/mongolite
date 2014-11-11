#include <Rinternals.h>
#include <bson.h>
#include <mongoc.h>

SEXP mkStringUTF8(const char* str);
SEXP mkRaw(const unsigned char *buf, int len);
bson_t* r2bson(SEXP ptr);
mongoc_collection_t* r2col(SEXP ptr);
SEXP bson2r(bson_t* b);
void fin_bson(SEXP ptr);
