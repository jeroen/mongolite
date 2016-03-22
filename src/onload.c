#include <R_ext/Rdynload.h>
#include <mongolite.h>

void mongolite_log_handler (mongoc_log_level_t log_level, const char *log_domain, const char *message, void *user_data) {
  switch (log_level) {
  case MONGOC_LOG_LEVEL_ERROR: //0
    stop(message);
    break;
  case MONGOC_LOG_LEVEL_CRITICAL: //1
  case MONGOC_LOG_LEVEL_WARNING: //2
    Rf_warningcall_immediate(R_NilValue, message);
    break;
  case MONGOC_LOG_LEVEL_MESSAGE: //3
  case MONGOC_LOG_LEVEL_INFO: //4
    Rprintf("Mongo Message: %s\n", message);
    break;
  case MONGOC_LOG_LEVEL_DEBUG: //5
  case MONGOC_LOG_LEVEL_TRACE: //6
  default:
    break;
  }
}

void R_init_mongolite(DllInfo *info) {
  static mongoc_log_func_t logfun = mongolite_log_handler;
  mongoc_init();
  mongoc_log_set_handler(logfun, NULL);
}

void R_unload_mongolite(DllInfo *info) {
  mongoc_cleanup();
}
