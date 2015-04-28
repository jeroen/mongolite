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

mongo_stream_in <- function(cur, handler = NULL, pagesize = 1000, verbose = TRUE, customOut = NULL){

  # Type validation
  stopifnot(is.null(handler) || is.function(handler))
  stopifnot(is.numeric(pagesize))
  stopifnot(is.logical(verbose))
  stopifnot(is.null(customOut) || is.function(customOut))

  # Default handler appends to big list
  count <- 0
  cb <- if(is.null(handler)){
    out <- new.env()
    function(x){
      if(length(x)){
        count <<- count + length(x)
        out[[as.character(count)]] <<- x
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
    if(length(page)){
      cb(page)
      if(verbose)
        cat("\r Found", count, "records...")
    }
    if(length(page) < pagesize)
      break
  }

  # new customOut param accepts user defined function to unpack a single document
  # from a collection, useful for documents with a nested structure
  if(!is.null(customOut) && is.function(customOut)){
    if(verbose) cat("\r Imported", count, "records. Converting to dataframe...\n")
    out <- as.list(out, sorted = FALSE)
    if(length(out) == 1){
      # add .progress bar or comment to console as documents are iterated through?
      df <- plyr::ldply(.data = out[[names(out)]], .fun = customOut)
    } else {
      df <- plyr::ldply(.data = out, .fun = function(onePage) {
        plyr::ldply(onePage, customOut)
      })
    }
    return(df)
  }

  if(is.null(handler)){
    if(verbose) cat("\r Imported", count, "records. Simplifying into dataframe...\n")
    out <- as.list(out, sorted = FALSE)
    post_process(unlist(out[order(as.numeric(names(out)))], FALSE, FALSE))
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
