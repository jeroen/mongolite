#' Standalone BSON reader
#'
#' Utility to read BSON data into R without MongoDB. Useful to read a dump from
#' from a `mongoexport` dump if it fits in memory. This utility does not attempt
#' to convert result into one giant data.frame: the output is a vector of length
#' equal to total number of documents in the bson collection. However it is
#' possible to simplify the separate data objects using [jsonlite][fromJSON] rules,
#' to recognize atomic vectors and data frames in nested bson documents.
#'
#' An alternative to this function is to import your bson dump into a local mongodb
#' server using the [mongo$import][mongo] function. This requires little memory
#' and once data is in mongodb you can easily query and manipulate it using this
#' package.
#'
#' @export
#' @useDynLib mongolite R_bson_reader_file
#' @param file path or url to a bson file
#' @param as_json read data into json strings instead of R lists.
#' @param simplify should nested data get simplified into atomic vectors and
#' dataframes where possible? See details.
#' @param verbose print some progress output while reading
#' @examples
#' diamonds <- read_bson("https://jeroen.github.io/data/diamonds.bson")
#' length(diamonds)
read_bson <- function(file, as_json = FALSE, simplify = TRUE, verbose = interactive()){
  if(grepl("^https?://", file)){
    file_url <- file
    file <- tempfile()
    on.exit(unlink(file))
    curl::curl_download(file_url, file, quiet = !isTRUE(verbose))
  }
  file <- normalizePath(file, mustWork = TRUE)
  out <- .Call(R_bson_reader_file, file, as_json, verbose)
  if(!isTRUE(simplify))
    return(out)
  if(verbose)
    cat("Simplifying data...\n", file = stderr())
  lapply(out, jsonlite:::simplify)
}
