#' Standalone BSON reader
#'
#' Utility to read BSON data into R without MongoDB. Useful to read a dump from
#' from a `mongoexport` dump if it fits in memory. This utility does not attempt
#' to convert data into a data.frame: the output is a vector with length equal
#' to the number of documents in the collection.
#'
#' Alternatively, to import a bson dump into your local mongodb server, use the
#' [mongo$import][mongo] function instead. This requires little memory and once
#' data is in mongo you can easily query it.
#'
#' @export
#' @useDynLib mongolite R_bson_reader_file
#' @param file path to a bson file on disk
#' @param as_json read data into json strings instead of R lists.
#' @param verbose print some progress output while reading
#' @examples
#' download.file("http://jeroen.github.io/data/diamonds.bson", "diamonds.bson")
#' diamonds <- read_bson("diamonds.bson")
#' unlink("diamonds.bson")
read_bson <- function(file, as_json = FALSE, verbose = interactive()){
  file <- normalizePath(file, mustWork = TRUE)
  .Call(R_bson_reader_file, file, as_json, verbose)
}
