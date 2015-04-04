#' @importFrom jsonlite toJSON fromJSON
mongo_stream_out <- function(data, mongo, pagesize = 1000, verbose = TRUE, ...){
  stopifnot(is.data.frame(data))
  FUN <- function(x){
    mongo_collection_insert_page(mongo, jsonlite:::asJSON(x, digits = 9, collapse = FALSE), ...)
  }
  jsonlite:::apply_by_pages(data, FUN, pagesize = pagesize, verbose = verbose)
}

mongo_stream_in <- function(mongo, handler = NULL, pagesize = 1000, verbose = TRUE,
  query = '{}', fields = '{"_id" : 0}', skip = 0, limit = 0){

  # Default handler appends to big list
  count <- 0
  cb <- if(is.null(handler)){
    estimate <- mongo_collection_count(mongo, query = query) - skip
    if(limit > 0){
      estimate <- min(limit, estimate)
    }
    out <- vector(mode = "list", length = estimate)
    function(x){
      if(length(x)){
        from <- count+1
        count <<- count + length(x)
        out[from:count] <<- x
      }
    }
  } else {
    function(x){
      handler(jsonlite:::simplify(x))
      count <<- count + length(x)
    }
  }

  # Query
  cur <- mongo_collection_find(mongo, query = query, fields = fields, skip = skip, limit = limit)

  # Read data page by page
  repeat {
    page <- mongo_cursor_next_page(cur, pagesize)
    if(is.null(page[[1]])){
      break
    } else if(is.null(page[[pagesize]])) {
      cb(Filter(is.list, page))
      break
    } else {
      cb(page)
    }
    if(verbose) {
      cat("\r Found", count, "records...")
    }
  }
  if(verbose){
    cat("\r Done! Imported", count, "records.\n")
  }

  if(is.null(handler)){
    if(count != estimate){
      message("Database state has changed while retrieving records.")
      out <- Filter(function(x){!is.null(x)}, out)
    }
    if(verbose) message("Simplifying into dataframe...")
    as.data.frame(jsonlite:::simplify(out))
  } else {
    invisible()
  }
}
