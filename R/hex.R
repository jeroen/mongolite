#' @useDynLib mongolite R_parse_hex_string
hex2bin <- function(string){
  .Call(R_parse_hex_string, string)
}
