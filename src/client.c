#include <mongolite.h>

#define safe_string(x) x ? Rf_mkString(x) : R_NilValue

SEXP R_mongo_client_server_status(SEXP ptr_client) {
  bson_t reply;
  bson_error_t err;
  mongoc_client_t *client = r2client(ptr_client);
  if(!mongoc_client_get_server_status(client, NULL, &reply, &err))
    stop(err.message);
  return bson2list(&reply);
}

SEXP R_default_ssl_options(){
  const mongoc_ssl_opt_t *opt = mongoc_ssl_opt_get_default();
  return Rf_list5(
    safe_string(opt->pem_file),
    safe_string(opt->ca_file),
    safe_string(opt->ca_dir),
    safe_string(opt->crl_file),
    ScalarLogical(opt->weak_cert_validation)
  );
}

SEXP R_mongo_client_new(SEXP uri_string, SEXP pem_file, SEXP pem_pwd, SEXP ca_file,
                        SEXP ca_dir, SEXP crl_file, SEXP weak_cert_validation) {
  mongoc_client_t *client = mongoc_client_new (translateCharUTF8(asChar(uri_string)));
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
  if(Rf_length(weak_cert_validation))
    opt.weak_cert_validation = asLogical(weak_cert_validation);

  if (mongoc_uri_get_ssl (mongoc_client_get_uri(client))) {
    mongoc_client_set_ssl_opts(client, &opt);
  }

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
