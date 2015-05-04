#include <mongolite.h>

SEXP R_mongo_init() {
  static mongoc_log_func_t logfun = mongolite_log_handler;
  mongoc_init();
  mongoc_log_set_handler(logfun, NULL);
  return R_NilValue;
}

SEXP R_mongo_cleanup() {
  mongoc_cleanup();
  return R_NilValue;
}

void mongolite_log_handler (mongoc_log_level_t log_level, const char *log_domain, const char *message, void *user_data) {
  switch (log_level) {
    case MONGOC_LOG_LEVEL_ERROR:
      stop(message);
      break;
    case MONGOC_LOG_LEVEL_CRITICAL:
    case MONGOC_LOG_LEVEL_WARNING:
      Rf_warningcall_immediate(R_NilValue, message);
      break;
    case MONGOC_LOG_LEVEL_MESSAGE:
    case MONGOC_LOG_LEVEL_INFO:
      Rprintf("Mongo Message: %s\n", message);
      break;
    case MONGOC_LOG_LEVEL_DEBUG:
    case MONGOC_LOG_LEVEL_TRACE:
    default:
      break;
  }
}
