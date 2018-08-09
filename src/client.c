#include <mongolite.h>

#define safe_string(x) x ? Rf_mkString(x) : R_NilValue

SEXP R_default_ssl_options(){
  const mongoc_ssl_opt_t *opt = mongoc_ssl_opt_get_default();
  SEXP out = PROTECT(Rf_allocVector(VECSXP, 6));
  SET_VECTOR_ELT(out, 0, safe_string(opt->pem_file));
  SET_VECTOR_ELT(out, 1, safe_string(opt->ca_file));
  SET_VECTOR_ELT(out, 2, safe_string(opt->ca_dir));
  SET_VECTOR_ELT(out, 3, safe_string(opt->crl_file));
  SET_VECTOR_ELT(out, 4, Rf_ScalarLogical(opt->weak_cert_validation));
  SET_VECTOR_ELT(out, 5, Rf_ScalarLogical(opt->weak_cert_validation));
  UNPROTECT(1);
  return out;
}

SEXP R_mongo_client_new(SEXP uri_string, SEXP pem_file, SEXP pem_pwd, SEXP ca_file,
                        SEXP ca_dir, SEXP crl_file, SEXP allow_invalid_hostname, SEXP weak_cert_validation) {

  bson_error_t err;
  mongoc_uri_t *uri = mongoc_uri_new_with_error (Rf_translateCharUTF8(STRING_ELT(uri_string, 0)), &err);
  if (!uri)
    Rf_error("failed to parse URI: %s (%s)", uri_string, err.message);

  mongoc_client_t *client = mongoc_client_new_from_uri (uri);
  if(!client)
    stop("Invalid uri_string. Try mongodb://localhost");

  //set ssl certificates here
#ifdef MONGOC_ENABLE_SSL
  mongoc_ssl_opt_t opt = { 0 };
  if(Rf_length(pem_file))
    opt.pem_file = CHAR(STRING_ELT(pem_file, 0));
  if(Rf_length(pem_pwd))
    opt.pem_pwd = CHAR(STRING_ELT(pem_pwd, 0));
  if(Rf_length(ca_file))
    opt.ca_file = CHAR(STRING_ELT(ca_file, 0));
  if(Rf_length(ca_dir))
    opt.ca_dir = CHAR(STRING_ELT(ca_dir, 0));
  if(Rf_length(crl_file))
    opt.crl_file = CHAR(STRING_ELT(crl_file, 0));
  if(Rf_length(allow_invalid_hostname))
    opt.allow_invalid_hostname = Rf_asLogical(allow_invalid_hostname);
  if(Rf_length(weak_cert_validation))
    opt.weak_cert_validation = Rf_asLogical(weak_cert_validation);

  if (mongoc_uri_get_ssl (mongoc_client_get_uri(client))) {
    mongoc_client_set_ssl_opts(client, &opt);
  }

  mongoc_client_set_appname (client, "r/mongolite");

#endif


  //verify that server is online
  //can fail if user has limited priv
  /*
  bson_error_t err;
  if(!mongoc_client_get_server_status(client, NULL, NULL, &err)){
    mongoc_client_destroy(client);
    stop(err.message);
  }
  */

  return client2r(client);
}
