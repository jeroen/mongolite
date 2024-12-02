#' Standalone BSON reader
#'
#' Utility to read BSON data into R without MongoDB. Useful to read a dump from
#' from a `mongoexport` dump if it fits in memory. This utility does not attempt
#' to convert data into a data.frame: the output is a vector with length of the
#' total number of documents in the bson collection.
#'
#' Alternatively, to import a bson dump into your local mongodb server, use the
#' [mongo$import][mongo] function instead. This requires little memory and once
#' data is in mongo you can easily query it.
#'
#' @export
#' @useDynLib mongolite R_bson_reader_file
#' @param file path or url to a bson file
#' @param as_json read data into json strings instead of R lists.
#' @param verbose print some progress output while reading
#' @examples
#' diamonds <- read_bson("http://jeroen.github.io/data/diamonds.bson")
#' length(diamonds)
read_bson <- function(file, as_json = FALSE, verbose = interactive()){
  if(grepl("^https?://", file)){
    file_url <- file
    file <- tempfile()
    on.exit(unlink(file))
    curl::curl_download(file_url, file, quiet = !isTRUE(verbose))
  }
  file <- normalizePath(file, mustWork = TRUE)
  .Call(R_bson_reader_file, file, as_json, verbose)
}
