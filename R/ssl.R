#' SSL options
#'
#' Set SSL options to connect to the server.
#'
#' @useDynLib mongolite R_default_ssl_options
#' @export
#' @param cert path to PEM file with client certificate, or a certificate as
#' returned by \link[openssl:read_cert]{openssl::read_cert}
#' @param key path to PEM file with private key from the above certificate, or
#' a key as returned by \link[openssl:read_key]{openssl::read_key}. This can
#' be the same PEM file as cert.
#' @param ca a certificate authority PEM file
#' @param ca_dir directory with CA files
#' @param crl_file file with revokations
#' @param weak_cert_validation disable certificate verification
ssl_options <- function(cert = NULL, key = cert, ca = NULL, ca_dir = NULL,
                        crl_file = NULL, weak_cert_validation = NULL){
  opts <- .Call(R_default_ssl_options)
  names(opts) <- c("pem_file", "ca_file", "ca_dir", "crl_file", "weak_cert_validation")
  if(length(cert)){
    key <- openssl::read_key(key)
    cert <- openssl::read_cert(cert)
    if(!identical(as.list(key)$pubkey, as.list(cert)$pubkey))
      warning("Key does not seem to match certificate!")
    tmp <- tempfile()
    passwd <- paste(openssl::sha1(openssl::rand_bytes(100)), collapse = "")
    writeLines(c(openssl::write_pem(key, password = passwd), openssl::write_pem(cert)), con = tmp)
    opts$pem_file = tmp
    opts$pem_pwd = passwd
  }
  if(length(ca))
    opts$ca_file = normalizePath(ca, mustWork = TRUE)
  if(length(ca_dir))
    opts$ca_dir = normalizePath(ca_dir, mustWork = TRUE)
  if(length(crl_file))
    opts$crl_file = normalizePath(crl_file, mustWork = TRUE)
  if(length(weak_cert_validation))
    opts$weak_cert_validation = weak_cert_validation
  return(opts)
}
