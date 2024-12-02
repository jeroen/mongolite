#' Standalone BSON reader
#'
#' Utility to parse BSON data into R without using MongoDB. Useful to read data
#' from a `mongoexport` dump without mongodb if it fits in memory.
#'
#' To import a bson dump into a local mongodb server, use the [mongo$import][mongo]
#' function instead. This requires less memory and once data is in mongo you can
#' easily query it.
#'
#' @export
#' @useDynLib mongolite R_bson_reader_new R_bson_reader_read
#' @param con either a path to a file, a url, or a a connection object
#' @param as_json return data as json strings instead of R lists
#' @param verbose print some output as we read
#' @return list with either data objects or json strings
#' @examples
#' diamonds <- read_bson("http://jeroen.github.io/data/diamonds.bson")
read_bson <- function(con, as_json = FALSE, verbose = interactive()){
  if(length(con) && is.character(con)){
    con <- if(grepl("^https?://", con)){
      url(con)
    } else {
      file(normalizePath(con, mustWork = TRUE), raw = TRUE)
    }
  }
  stopifnot(inherits(con, 'connection'))
  open(con, 'rb')
  on.exit(close(con))
  reader <- .Call(R_bson_reader_new, con)
  output <- new.env(parent = emptyenv())
  i <- 0
  one <- function(as_json = FALSE){
    .Call(R_bson_reader_read, reader, as_json)
  }
  while(length(obj <- one(as_json))){
    i <- i+1
    if(isTRUE(verbose))
      cat("\rRead", i, file = stderr())
    output[[sprintf('%09d', i)]] <- obj
  }
  if(isTRUE(verbose))
    cat("\rDone!\n", file = stderr())
  unname(as.list(output, sorted = TRUE))
}
