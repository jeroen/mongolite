#' GridFS API
#'
#' Connect to a GridFS database to search, read, write and delete files.
#'
#' We support two interfaces for sending/receiving data from/to GridFS. The
#' `fs$read()` and `fs$write()` methods are the most flexible and can send data
#' from/to an R connection, such as a [file][file], [socket][socketConnection]
#' or [url][url]. These methods support a progress counter and can be interrupted
#' if needed. These methods are recommended for reading or writing single files.
#'
#' The `fs$upload()` and `fs$download()` methods on the other hand copy directly
#' between GridFS and your local disk. This API is vectorized so it can transfer
#' many files at once. However individual transfers cannot be interrupted and will
#' block R until completed. This API is only recommended to upload/download a large
#' number of small files.
#'
#' Modifying files in GridFS is currently unsupported: uploading a file with the
#' same name will generate a new file.
#'
#' @inheritParams mongo
#' @export
#' @param prefix string to prefix the collection name
#' @examples # Upload a file to GridFS
#' fs <- gridfs(url = "mongodb+srv://readwrite:test@cluster0-84vdt.mongodb.net/test")
#' input <- file.path(R.home('doc'), "html/logo.jpg")
#' fs$upload(input, name = 'logo.jpg')
#'
#' # Download the file back to disk
#' output <- file.path(tempdir(), 'logo1.jpg')
#' fs$download('logo.jpg', output)
#'
#' # Or you can also stream it
#' con <- file(file.path(tempdir(), 'logo2.jpg'))
#' fs$read('logo.jpg', con)
#'
#' # Delete the file on the server
#' fs$remove('logo.jpg')
#'
#' files <- c(input, file.path(tempdir(), c('logo1.jpg', 'logo2.jpg')))
#' hashes <- tools::md5sum(files)
#' stopifnot(length(unique(hashes)) == 1)
#'
#' \donttest{
#' # Insert Binary Data
#' fs <- gridfs()
#' buf <- serialize(nycflights13::flights, NULL)
#' fs$write(buf, 'flights')
#' out <- fs$read('flights')
#' flights <- unserialize(out$data)
#'
#' tmp <- file.path(tempdir(), 'flights.rds')
#' fs$download('flights', tmp)
#' flights2 <- readRDS(tmp)
#' stopifnot(all.equal(flights, nycflights13::flights))
#' stopifnot(all.equal(flights2, nycflights13::flights))
#'
#' # Show what we have
#' fs$find()
#' fs$drop()
#' }
#' @section Methods:
#' \describe{
#'   \item{\code{find(filter = "{}", options = "{}")}}{Search and list files in the GridFS}
#'   \item{\code{download(name, path = '.')}}{Download one or more files from GridFS to disk. Path may be an existing directory or vector of filenames equal to 'name'.}
#'   \item{\code{upload(path, name = basename(path), content_type = NULL, metadata = NULL)}}{Upload one or more files from disk to GridFS. Metadata is an optional JSON string.}
#'   \item{\code{read(name, con = NULL, progress = TRUE)}}{Reads a single file from GridFS into a writable R [connection].
#'   If `con` is a string it is treated as a filepath; if it is `NULL` then the output is buffered in memory and returned as a [raw] vector.}
#'   \item{\code{write(con, name, content_type = NULL, metadata = NULL, progress = TRUE)}}{Stream write a single file into GridFS from a readable R [connection].
#'   If `con` is a string it is treated as a filepath; it may also be a [raw] vector containing the data to upload. Metadata is an optional JSON string.}
#'   \item{\code{remove(name)}}{Remove a single file from the GridFS}
#'   \item{\code{drop()}}{Removes the entire GridFS collection, including all files}
#' }
gridfs <- function(db = "test", url = "mongodb://localhost", prefix = "fs", options = ssl_options()){
  client <- new_client(c(list(uri = url), options))

  # Get a database
  if(missing(db) || is.null(db)){
    url_db <- mongo_get_default_database(client)
    if(length(url_db) && nchar(url_db))
      db <- url_db
  }

  fs <- mongo_gridfs_new(client, prefix, db)
  orig <- list(
    prefix = prefix,
    db = db,
    url = url,
    options = options
  )
  if(length(options$pem_file) && file.exists(options$pem_file))
    attr(orig, "pemdata") <- readLines(options$pem_file)

  rm(client) #needed for m$disconnect() to work
  fs_object(fs, orig)
}

fs_object <- function(fs, orig){
  check_fs <- function(){
    if(null_ptr(fs)){
      message("Connection lost. Trying to reconnect with mongo...")
      fs <<- gridfs_reset(orig)
    }
  }

  self <- local({
    drop <- function(){
      check_fs()
      mongo_gridfs_drop(fs)
    }
    find <-  function(filter = '{}', options = '{}'){
      check_fs()
      mongo_gridfs_find(fs, filter, options)
    }
    upload <- function(path, name = basename(path), content_type = NULL, metadata = NULL){
      check_fs()
      mongo_gridfs_upload(fs, name, path, content_type, metadata)
    }
    download <- function(name, path = "."){
      check_fs()
      mongo_gridfs_download(fs, name, path)
    }
    read <- function(name, con = NULL, progress = TRUE){
      check_fs()
      mongo_gridfs_read_stream(fs, name, con, progress)
    }
    write <- function(con, name, content_type = NULL, metadata = NULL, progress = TRUE){
      check_fs()
      mongo_gridfs_write_stream(fs, name, con, content_type, metadata, progress)
    }
    remove <- function(name){
      check_fs()
      mongo_gridfs_remove(fs, name)
    }
    disconnect <- function(gc = TRUE){
      mongo_gridfs_disconnect(fs)
      if(isTRUE(gc))
        base::gc()
      invisible()
    }
    environment()
  })
  lockEnvironment(self, TRUE)
  structure(self, class=c("gridfs", "jeroen", class(self)))
}

gridfs_reset <- function(orig){
  if(length(orig$options$pem_file) && !file.exists(orig$options$pem_file)){
    orig$options$pem_file <- tempfile()
    writeLines(attr(orig, "pemdata"), orig$options$pem_file)
  }
  client <- new_client(c(list(uri = orig$url), orig$options))
  mongo_gridfs_new(client, prefix = orig$prefix, db = orig$db)
}

#' @useDynLib mongolite R_mongo_gridfs_new
mongo_gridfs_new <- function(client, prefix, db){
  .Call(R_mongo_gridfs_new, client, prefix, db)
}

#' @useDynLib mongolite R_mongo_gridfs_drop
mongo_gridfs_drop <- function(fs){
  .Call(R_mongo_gridfs_drop, fs)
}

#' @useDynLib mongolite R_mongo_gridfs_find
mongo_gridfs_find <- function(fs, filter, opts){
  out <- .Call(R_mongo_gridfs_find, fs, bson_or_json(filter), bson_or_json(opts))
  list_to_df(out)
}

#' @useDynLib mongolite R_mongo_gridfs_disconnect
mongo_gridfs_disconnect <- function(fs){
  stopifnot(inherits(fs, "mongo_gridfs"))
  .Call(R_mongo_gridfs_disconnect, fs)
}

#' @useDynLib mongolite R_mongo_gridfs_upload
mongo_gridfs_upload <- function(fs, name, path, type, metadata){
  stopifnot(is.character(name))
  path <- normalizePath(path, mustWork = TRUE)
  is_dir <- file.info(path)$isdir
  if(any(is_dir))
    stop(sprintf("Upload contains directories, you can only upload files (%s)", paste(path[is_dir], collapse = ", ")))
  stopifnot(length(name) == length(path))
  id <- rep(NA, length(name))
  if(is.null(type))
    type <- mime::guess_type(name, unknown = NA, empty = NA)
  type <- as.character(rep_len(type, length(name)))
  metadata <- if(length(metadata))
    bson_or_json(metadata)
  out <- vector("list", length(name))
  for(i in seq_along(name)){
    out[[i]] <- .Call(R_mongo_gridfs_upload, fs, name[i], path[i], type[i], metadata)
  }
  df <- list_to_df(out)
  df$path = path
  df
}

#' @useDynLib mongolite R_mongo_gridfs_download
mongo_gridfs_download <- function(fs, name, path){
  if(length(path) == 1 && isTRUE(file.info(path)$isdir)){
    path <- normalizePath(file.path(path, name), mustWork = FALSE)
  } else if(length(name) != length(path)){
    stop("Argument 'path' must be an existing dir or vector of filenames equal length as 'name'")
  }
  path <- normalizePath(path, mustWork = FALSE)
  lapply(path, function(x){ dir.create(dirname(x), showWarnings = FALSE, recursive = TRUE)})
  stopifnot(length(name) == length(path))
  out <- vector("list", length(name))
  for(i in seq_along(name)){
    out[[i]] <- .Call(R_mongo_gridfs_download, fs, name_or_query(name[i]), path[i])
  }
  df <- list_to_df(out)
  df$path = path
  df
}

#' @useDynLib mongolite R_mongo_gridfs_remove
mongo_gridfs_remove <- function(fs, name){
  out <- lapply(name, function(x){
    .Call(R_mongo_gridfs_remove, fs, name_or_query(x))
  })
  list_to_df(out)
}

#' @useDynLib mongolite R_new_read_stream R_stream_read_chunk R_stream_close
mongo_gridfs_read_stream <- function(fs, name, con, progress = TRUE){
  name <- name_or_query(name)
  stream <- .Call(R_new_read_stream, fs, name)
  size <- attr(stream, 'size')
  if(length(con) && is.character(con))
    con <- file(con, raw = TRUE)
  if(!length(con)){
    con <- rawConnection(raw(size), 'wb')
    on.exit(close(con))
  }
  stopifnot(inherits(con, "connection"))
  if(!isOpen(con)){
    open(con, 'wb')
    on.exit(close(con))
  }
  remaining <- size
  bufsize <- 1024 * 1024
  while(remaining > 0){
    buf <- .Call(R_stream_read_chunk, stream, bufsize)
    remaining <- remaining - length(buf)
    if(length(buf) < bufsize && remaining > 0)
      stop("Stream read incomplete: ", remaining, " remaining")
    writeBin(buf, con)
    if(isTRUE(progress))
      cat(sprintf("\r[%s]: read %s (%d%%)     ", name, as_size(size - remaining), as.integer(100 * (size - remaining) / size)))
  }
  if(isTRUE(progress))
    cat(sprintf("\r[%s]: read %s (done)\n", name, as_size(size - remaining)))
  out <- .Call(R_stream_close, stream)
  if(inherits(con, 'rawConnection'))
    out$data <- rawConnectionValue(con)
  structure(out, class = "miniprint")
}

#' @useDynLib mongolite R_new_write_stream R_stream_write_chunk R_stream_close
mongo_gridfs_write_stream <- function(fs, name, con, type, metadata, progress = TRUE){
  stopifnot(is.character(name))
  type <- as.character(type)
  metadata <- if(length(metadata))
    bson_or_json(metadata)
  stream <- .Call(R_new_write_stream, fs, name, type, metadata)
  if(length(con) && is.character(con)){
    con <- if(grepl("^https?://", con)){
      url(con)
    } else {
      file(con, raw = TRUE)
    }
  }
  if(is.raw(con)){
    con <- rawConnection(con, 'rb')
    on.exit(close(con))
  }
  stopifnot(inherits(con, "connection"))
  if(!isOpen(con)){
    open(con, 'rb')
    on.exit(close(con))
  }
  total <- 0
  bufsize <- 1024 * 1024
  repeat {
    buf <- readBin(con, raw(), bufsize)
    total <- total + .Call(R_stream_write_chunk, stream, buf)
    if(!length(buf))
      break
    if(isTRUE(progress))
      cat(sprintf("\r[%s]: written %s     ", name, as_size(total)))
  }
  if(isTRUE(progress))
    cat(sprintf("\r[%s]: written %s (done)\n", name, as_size(total)))
  out <- .Call(R_stream_close, stream)
  structure(out, class = "miniprint")
}

as_size <- function(n) {
  format(structure(n, class="object_size"), units="auto", standard = "SI", digits = 2L)
}

list_to_df <- function(list, cols = c("id", "name", "size", "date", "type", "metadata")){
  out <- lapply(cols, function(col){
    sapply(list, `[[`, col)
  })
  df <- structure(out, class="data.frame", names = cols, row.names = seq_along(list))
  class(df$date) = c("POSIXct", "POSIXt")
  df
}

name_or_query <- function(x){
  if(!is.character(x)){
    stop("Parameter 'name' must be a json query or filename (without spaces)")
  }
  if(grepl("^id:", x)){
    x <- sprintf('{"_id": {"$oid":"%s"}}', sub("^id:", "", x))
  }
  if(jsonlite::validate(x)){
    return(bson_or_json(x))
  } else {
    if(grepl("[\t {]", x)){
      stop("Parameter 'name' does not contain valid json or filename (no spaces)")
    }
    return(x)
  }
}
