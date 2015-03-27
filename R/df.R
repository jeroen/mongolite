#' @export
#' @importFrom jsonlite toJSON fromJSON unbox
mongo_write_df <- function(m, data, verbose = TRUE){
  jsonlines <- jsonlite:::asJSON(data, collapse = FALSE)
  if(verbose) cat("Importing...")
  vapply(jsonlines, function(doc){
    mongo_collection_insert(m, doc)
    if(verbose) cat(".")
    TRUE
  }, logical(1))
  cat("done!")
}

#' @export
mongo_read_df <- function(m, verbose = TRUE){
  cur <- mongo_collection_find(m)
  i <- 0
  out <- list()
  if(verbose) cat("Reading...")

  #mongo_cursor_more seems broken
  while(!is.null(val <- mongo_cursor_next(cur))) {
    i <- i+1
    if(verbose) cat(".")
    out[[i]] <- bson_to_list(val)
  }
  if(verbose) cat(".")
  jsonlite:::simplify(out)
}
