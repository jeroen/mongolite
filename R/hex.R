#' Get OID date
#'
#' The initial 4 bytes of a MongoDB OID contain a timestamp value, representing
#' the ObjectId creation, measured in seconds since the Unix epoch.
#'
#' @export
#' @param string of length 24 containing the OID
#' @examples oid_to_timestamp('5349b4ddd2781d08c09890f3')
oid_to_timestamp <- function(oid){
  stopifnot("oid invalid" = nchar(oid) > 23)
  val <- strtoi(paste0('0x', substring(oid, 1, 8)))
  structure(val, class = c("POSIXct", "POSIXt"))
}

#' @useDynLib mongolite R_parse_hex_string
hex2bin <- function(string){
  .Call(R_parse_hex_string, string)
}
