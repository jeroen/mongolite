#' @export
#' @importFrom jsonlite toJSON fromJSON unbox
#' @importFrom utils txtProgressBar setTxtProgressBar
mongo_stream_out <- function(m, data, verbose = TRUE){
  stopifnot(is.data.frame(data))
  n <- nrow(data)
  jsonlines <- jsonlite:::asJSON(data, collapse = FALSE)
  if(verbose){
    pb <- txtProgressBar(style = 3)
    on.exit(close(pb))
  }
  for(i in seq_len(n)){
    mongo_collection_insert(m, jsonlines[i])
    if(verbose) setTxtProgressBar(pb, i/n)
  }
  invisible()
}

#' @export
mongo_stream_in <- function(m, ..., verbose = TRUE){
  cur <- mongo_collection_find(m, ...)
  i <- 0
  out <- list()

  # Rstudio bug
  if(verbose) cat("\n")

  # The mongo_cursor_more function seems broken
  while(!is.null(val <- mongo_cursor_next(cur))) {
    i <- i+1
    if(verbose && (i %% 100 == 0)){
      cat("\r Found", i, "records.")
    }
    out[[i]] <- bson_to_list(val)
  }
  if(verbose){
    cat("\r Finished", i, "records.\n")
  }
  jsonlite:::simplify(out)
}
