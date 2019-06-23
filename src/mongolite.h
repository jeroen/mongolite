#define R_NO_REMAP
#define STRICT_R_HEADERS

#include <Rinternals.h>
#include <bson/bson.h>
#include <mongoc/mongoc.h>

#define stop(...) Rf_errorcall(R_NilValue, __VA_ARGS__)

SEXP mkStringUTF8(const char* str);
SEXP mkRaw(const unsigned char *buf, int len);
bson_t* r2bson(SEXP ptr);
mongoc_collection_t* r2col(SEXP ptr);
mongoc_cursor_t* r2cursor(SEXP ptr);
mongoc_client_t* r2client(SEXP ptr);
mongoc_gridfs_t* r2gridfs(SEXP ptr);
SEXP bson2r(bson_t* b);
SEXP col2r(mongoc_collection_t *col, SEXP prot);
SEXP cursor2r(mongoc_cursor_t* c, SEXP prot);
SEXP client2r(mongoc_client_t *client);
SEXP gridfs2r(mongoc_gridfs_t *fs, SEXP prot);
void mongolite_log_handler (mongoc_log_level_t log_level, const char *log_domain, const char *message, void *user_data);
SEXP ConvertObject(bson_iter_t* iter, bson_iter_t* counter);
SEXP bson2list(bson_t *b);
SEXP bson_to_str(const bson_t * b);
