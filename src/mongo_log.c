#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include <Rversion.h>
#include <mongolite.h>

//default
mongoc_log_level_t max_log_level = MONGOC_LOG_LEVEL_INFO;

SEXP R_mongo_log_level(SEXP level){
  if(level != R_NilValue)
    max_log_level = Rf_asInteger(level);
  return Rf_mkString(mongoc_log_level_str(max_log_level));
}

void mongolite_log_handler (mongoc_log_level_t event, const char *log_domain, const char *message, void *user_data) {
  if(event > max_log_level)
    return;
  switch (event) {
  case MONGOC_LOG_LEVEL_ERROR: //0
  case MONGOC_LOG_LEVEL_CRITICAL: //1
  case MONGOC_LOG_LEVEL_WARNING: //2
    Rf_warningcall_immediate(R_NilValue, "[%s] %s", mongoc_log_level_str(event), message);
    break;
  case MONGOC_LOG_LEVEL_MESSAGE: //3
  case MONGOC_LOG_LEVEL_INFO: //4
  case MONGOC_LOG_LEVEL_DEBUG: //5
  case MONGOC_LOG_LEVEL_TRACE: //6
    Rprintf("Mongo Message: %s\n", message);
    break;
  default:
    break;
  }
}

void R_init_mongolite(DllInfo *info) {
  static mongoc_log_func_t logfun = mongolite_log_handler;
  char *r_version;
  mongoc_init();

  SEXP agent = GetOption1(install("HTTPUserAgent"));
  if (isString(agent) && Rf_length(agent)) {
    r_version = bson_strdup_printf ("%s ", CHAR(STRING_ELT(agent, 0)));
  }
  else {
#if defined (R_VERSION)
  r_version = bson_strdup_printf ("R=%s.%s ", R_MAJOR, R_MINOR);
#else
  r_version = bson_strdup ("R=undefined ");
#endif
  }
  mongoc_handshake_data_append ("mongolite", "", r_version);
  mongoc_log_set_handler(logfun, NULL);
  R_registerRoutines(info, NULL, NULL, NULL, NULL);
  R_useDynamicSymbols(info, TRUE);
  bson_free (r_version);
}

void R_unload_mongolite(DllInfo *info) {
  mongoc_cleanup();
}
