#' @importFrom jsonlite toJSON fromJSON
mongo_stream_out <- function(data, mongo, pagesize = 1000, verbose = TRUE){
  stopifnot(is.data.frame(data))
  stopifnot(is.numeric(pagesize))
  stopifnot(is.logical(verbose))
  FUN <- function(x){
    mongo_collection_insert_page(mongo, jsonlite:::asJSON(x, digits = 9, collapse = FALSE))
  }
  jsonlite:::apply_by_pages(data, FUN, pagesize = pagesize, verbose = verbose)
  TRUE
}

mongo_stream_in <- function(cur, handler = NULL, pagesize = 1000, verbose = TRUE){

  # Type validation
  stopifnot(is.null(handler) || is.function(handler))
  stopifnot(is.numeric(pagesize))
  stopifnot(is.logical(verbose))

  # Default handler appends to big list
  count <- 0
  cb <- if(is.null(handler)){
    out <- new.env()
    function(x){
      if(length(x)){
        count <<- count + length(x)
        out[[sprintf("%010d", count)]] <<- x
      }
    }
  } else {
    function(x){
      handler(post_process(x))
      count <<- count + length(x)
    }
  }

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

  if(is.null(handler)){
    if(verbose) cat("\r Imported", count, "records. Simplifying into dataframe...\n")
    out <- unlist(lapply(sort(ls(out)), get, out, inherits = FALSE), FALSE, FALSE)
    post_process(out)
  } else {
    invisible()
  }
}

post_process <- function(x){
  df <- as.data.frame(jsonlite:::simplify(x))
  idcol <- match("_id", names(df))
  if(!is.na(idcol)){
    df[[idcol]] <- vapply(df[[idcol]], function(x){paste(format(x), collapse="")}, character(1))
  }
  df
}
