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
#' @examples # New GridFS
#' fs <- gridfs(url = "mongodb+srv://readwrite:test@cluster0-84vdt.mongodb.net/test")
#' input <- R.home('doc/NEWS.pdf')
#' fs$upload(input)
#' fs$download('NEWS.pdf', file.path(tempdir(), 'output.pdf'))
#' con <- file(file.path(tempdir(), 'output2.pdf'))
#' fs$read('NEWS.pdf', con)
#' hashes <- tools::md5sum(c(input, file.path(tempdir(), c('output.pdf', 'output2.pdf'))))
#' unlink(file.path(tempdir(), c('output.pdf', 'output2.pdf')))
#' stopifnot(length(unique(hashes)) == 1)
#'
#' # Insert Binary Data
#' fs$write('iris3', serialize(datasets::iris3, NULL))
#' out <- fs$read('iris3')
#' iris4 <- unserialize(out$data)
#' stopifnot(all.equal(iris4, datasets::iris3))
#'
#' # Show what we have
#' fs$find()
#' fs$drop()
#' @section Methods:
#' \describe{
#'   \item{\code{find(filter = "{}", options = "{}")}}{Search and list files in the GridFS}
#'   \item{\code{download(name, path = name)}}{Download one or more files from GridFS to disk}
#'   \item{\code{upload(path, name = basename(path), content_type = NULL, metadata = NULL)}}{Upload one or more files from disk to GridFS. Metadata is an optional JSON string.}
#'   \item{\code{read(name, con = NULL, progress = TRUE)}}{Reads a single file from GridFS into a writable R [connection].
#'   If `con` is a string it is treated as a filepath; if it is `NULL` then the output is buffered in memory and returned as a [raw] vector.}
#'   \item{\code{write(name, con, content_type = NULL, metadata = NULL, progress = TRUE)}}{Stream write a single file into GridFS from a readable R [connection].
#'   If `con` is a string it is treated as a filepath; it may also be a [raw] vector containing the data to upload. Metadata is an optional JSON string.}
#'   \item{\code{remove(name)}}{Remove a single file from the GridFS}
#'   \item{\code{drop()}}{Removes the entire GridFS collection, including all files}
#' }
gridfs <- function(db = "test", url = "mongodb://localhost", prefix = "fs", options = ssl_options()){
  client <- do.call(mongo_client_new, c(list(uri = url), options))

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
  fs_object(fs, client, orig)
}

fs_object <- function(fs, client, orig){
  self <- local({
    drop <- function(){
      mongo_gridfs_drop(fs)
    }
    find <-  function(filter = '{}', options = '{}'){
      mongo_gridfs_find(fs, filter, options)
    }
    upload <- function(path, name = basename(path), content_type = NULL, metadata = NULL){
      mongo_gridfs_upload(fs, name, path, content_type, metadata)
    }
    download <- function(name, path = name){
      mongo_gridfs_download(fs, name, path)
    }
    read <- function(name, con = NULL, progress = TRUE){
      mongo_gridfs_read_stream(fs, name, con, progress)
    }
    write <- function(name, con, content_type = NULL, metadata = NULL, progress = TRUE){
      mongo_gridfs_write_stream(fs, name, con, content_type, metadata, progress)
    }
    remove <- function(name){
      mongo_gridfs_remove(fs, name)
    }
    environment()
  })
  lockEnvironment(self, TRUE)
  structure(self, class=c("gridfs", "jeroen", class(self)))
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
  data.frame(
    id = as.character(out[[1]]),
    name = as.character(out[[2]]),
    size = as.numeric(out[[3]]),
    date = structure(as.numeric(out[[4]]) / 1000, class = c("POSIXct", "POSIXt")),
    type = as.character(out[[5]]),
    stringsAsFactors = FALSE
  )
}

#' @useDynLib mongolite R_mongo_gridfs_upload
mongo_gridfs_upload <- function(fs, name, path, type, metadata){
  stopifnot(is.character(name))
  path <- normalizePath(path, mustWork = TRUE)
  stopifnot(length(name) == length(path))
  id <- rep(NA, length(name))
  if(is.null(type))
    type <- mime::guess_type(name, unknown = NA, empty = NA)
  type <- as.character(rep_len(type, length(name)))
  metadata <- if(length(metadata))
    bson_or_json(metadata)
  for(i in seq_along(name)){
    id[i] <- .Call(R_mongo_gridfs_upload, fs, name[i], path[i], type[i], metadata)
  }
  structure(id, names = name)
}

#' @useDynLib mongolite R_mongo_gridfs_download
mongo_gridfs_download <- function(fs, name, path){
  stopifnot(is.character(name))
  path <- normalizePath(path, mustWork = FALSE)
  stopifnot(length(name) == length(path))
  out <- rep(NA, length(name))
  for(i in seq_along(name)){
    res <- .Call(R_mongo_gridfs_download, fs, name[i], path[i])
    out[i] <- res$id
  }
  structure(out, names = name)
}

#' @useDynLib mongolite R_mongo_gridfs_remove
mongo_gridfs_remove <- function(fs, name){
  vapply(name, function(x){
    .Call(R_mongo_gridfs_remove, fs, x)
  }, character(1))
}

#' @useDynLib mongolite R_new_read_stream R_stream_read_chunk R_stream_close
mongo_gridfs_read_stream <- function(fs, name, con, progress = TRUE){
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
  return(out)
}

#' @useDynLib mongolite R_new_write_stream R_stream_write_chunk R_stream_close
mongo_gridfs_write_stream <- function(fs, name, con, type, metadata, progress = TRUE){
  stopifnot(is.character(name))
  type <- as.character(type)
  metadata <- if(length(metadata))
    bson_or_json(metadata)
  stream <- .Call(R_new_write_stream, fs, name, type, metadata)
  if(length(con) && is.character(con))
    con <- file(con, raw = TRUE)
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
  return(out)
}

as_size <- function(n) {
  format(structure(n, class="object_size"), units="auto", standard = "SI", digits = 2L)
}
