#' @importFrom jsonlite toJSON fromJSON
mongo_stream_out <- function(data, mongo, pagesize = 1000, verbose = TRUE, stop_on_error = TRUE, ...){
  stopifnot(is.data.frame(data))
  stopifnot(is.numeric(pagesize))
  stopifnot(is.logical(verbose))
  FUN <- function(x){
    mongo_collection_insert_page(mongo, mongo_to_json(x, collapse = FALSE, ...), stop_on_error = stop_on_error)
  }
  out <- jsonlite:::apply_by_pages(data, FUN, pagesize = pagesize, verbose = verbose)
  structure(list(
    nInserted = sum(vapply(out, `[[`, numeric(1), 'nInserted')),
    nMatched = sum(vapply(out, `[[`, numeric(1), 'nMatched')),
    nRemoved = sum(vapply(out, `[[`, numeric(1), 'nRemoved')),
    nUpserted = sum(vapply(out, `[[`, numeric(1), 'nUpserted')),
    writeErrors = do.call(c, lapply(out, `[[`, 'writeErrors'))
  ), class = "miniprint")
}

# Different defaults than jsonlite
mongo_to_json <- function(x, digits = 9, POSIXt = "mongo", raw = "mongo", always_decimal = TRUE, ...){
  jsonlite:::asJSON(x, digits = digits, POSIXt = POSIXt, raw = raw, always_decimal = always_decimal, no_dots = TRUE, ...)
}

mongo_stream_in <- function(cur, handler = NULL, pagesize = 1000, verbose = TRUE, flat = FALSE){

  # Type validation
  stopifnot(is.null(handler) || is.function(handler))
  stopifnot(is.numeric(pagesize))
  stopifnot(is.logical(verbose))

  # Default handler appends to big list
  count <- 0
  if (is.null(handler)) {
    out <- new.env()
  }

  # Read data page by page
  repeat {
    if (flat) {
      page <- mongo_cursor_next_page_flattened(cur, pagesize)
      new <- length(page[[1]])
    } else {
      page <- mongo_cursor_next_page(cur, pagesize)
      new <- length(page)
    }
    if (new) {
      count <- count + new
      if (is.null(handler)) {
        out[[as.character(count)]] <- page
      } else {
        handler(post_process(page))
      }
    }
    if (verbose) cat("\r Found", count, "records...")
    if(new < pagesize)
      break
  }

  if(is.null(handler)){
    if(verbose) cat("\r Imported", count, "records. Simplifying into dataframe...\n")
    out <- as.list(out, sorted = FALSE)
    out <- out[order(as.numeric(names(out)))]
    if (flat) {
      if (requireNamespace("dplyr", quietly = TRUE)) {
        out <- dplyr::bind_rows(out)
      } else if (requireNamespace("data.table", quietly = TRUE)) {
        out <- data.table::rbindlist(out, fill = TRUE)
      } else {
        # TODO: This is much, much slower. We might consider bundling an
        # rbindlist implementation suited to the inputs instead.
        out <- lapply(out, as.data.frame, stringsAsFactors = FALSE)
        out <- do.call("rbind", out)
      }
      # For compatibility with jsonlite:::simplifyDataFrame():
      attr(out, "row.names") <- seq_len(nrow(out))
      out
    } else {
      post_process(unlist(out, FALSE, FALSE))
    }
  } else {
    invisible()
  }
}

post_process <- function(x){
  df <- as.data.frame(jsonlite:::simplify(x))
  #idcol <- match("_id", names(df))
  #if(!is.na(idcol) && is.list(df[[idcol]]) && all(vapply(df[[idcol]], is.raw, logical(1)))){
  #  df[[idcol]] <- vapply(df[[idcol]], function(x){paste(format(x), collapse="")}, character(1))
  #}
  df
}

mongo_export <- function(col, con = stdout(), query, fields, sort, verbose = FALSE){
  stopifnot(inherits(con, "connection"))
  if(!isOpen(con)){
    open(con, "w")
    on.exit(close(con))
  }
  cur <- mongo_collection_find(col, query = query, fields = fields, sort = sort)
  count <- 0;
  pagesize <- 100
  repeat {
    page <- mongo_cursor_next_json(cur, n = pagesize)
    size <- length(page)
    writeLines(page, con)
    count <- count + size;
    if(verbose)
      cat("\rExported", count, "lines...")
    if(size < pagesize)
      break
  }
  if(verbose) cat("\rDone! Exported a total of", count, "lines.\n")
  invisible(count)
}

# Same as mongo_export but with (binary) bson output
mongo_dump <- function(col, con = stdout(), query, fields, sort, verbose = FALSE){
  stopifnot(inherits(con, "connection"))
  if(!isOpen(con)){
    open(con, "wb")
    on.exit(close(con))
  }
  cur <- mongo_collection_find(col, query = query, fields = fields, sort = sort)
  count <- 0
  pagesize <- 100
  repeat {
    page <- mongo_cursor_next_bsonlist(cur, n = pagesize)
    size <- length(page)
    lapply(page, writeBin, con = con)
    count <- count + size
    if(verbose)
      cat("\rExported", count, "lines...")
    if(size < pagesize)
      break
  }
  if(verbose) cat("\rDone! Exported a total of", count, "lines.\n")
  invisible(count)
}

mongo_import <- function(col, con, verbose = FALSE){
  stopifnot(inherits(con, "connection"))
  if(!isOpen(con)){
    open(con, "r")
    on.exit(close(con))
  }
  count <- 0;
  while(length(json <- readLines(con, n = 100))) {
    json <- Filter(function(x){!grepl("^\\s*$", x)}, json)
    if(!all(vapply(json, jsonlite::validate, logical(1))))
      stop("Invalid JSON. Data must be in newline delimited json format (http://ndjson.org/)")
    mongo_collection_insert_page(col, json)
    count <- count + length(json)
    if(verbose)
      cat("\rImported", count, "lines...")
  }
  if(verbose)
    cat("\rDone! Imported a total of", count, "lines.\n")
  invisible(count)
}
