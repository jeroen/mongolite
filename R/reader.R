#' Standalone BSON reader
#'
#' Reads BSON data from a `mongoexport` dump file directly into R (if it can fit
#' in memory). This utility does not attempt to convert result into one big single
#' data.frame: the output is always a vector of length equal to total number of
#' documents in the collection.
#'
#' It is enabled by default to simplify the individual data documents using the
#' same rules as [jsonlite][fromJSON]. This converts nested lists into atomic
#' vectors and data frames when possible, which makes data easier to work with in R.
#'
#' An alternative to this function is to import your BSON file into a local mongodb
#' server using the [mongo$import()][mongo] function. This requires little memory
#' and once data is in mongodb you can easily query and modify it.
#'
#' @export
#' @useDynLib mongolite R_bson_reader_file
#' @param file path or url to a bson file
#' @param as_json read data into json strings instead of R lists.
#' @param simplify should nested data get simplified into atomic vectors and
#' dataframes where possible? Only used for `as_json = FALSE`.
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
  if(isTRUE(as_json) || !isTRUE(simplify))
    return(out)
  if(verbose)
    cat("Simplifying data...\n", file = stderr())
  lapply(out, jsonlite:::simplify)
}
