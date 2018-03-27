#' GridFS API
#'
#' Connect to a GridFS database.
#'
#' @inheritParams mongo
#' @export
#' @param prefix string to prefix the collection name
#' @examples # New GridFS
#' fs <- gridfs(url = "mongodb+srv://readwrite:test@cluster0-84vdt.mongodb.net/test")
#' input <- R.home('doc/NEWS.pdf')
#' fs$upload(input)
#' fs$download('NEWS.pdf', 'output.pdf')
#' hashes <- tools::md5sum(c(input, 'output.pdf'))
#' unlink('output.pdf')
#' stopifnot(hashes[[1]] == hashes[[1]])
#'
#' # Insert Binary Data
#' fs$write('iris3', serialize(datasets::iris3, NULL))
#' out <- unserialize(fs$read('iris3'))
#' stopifnot(all.equal(out, datasets::iris3))
#'
#' # Show what we have
#' fs$list()
#' fs$drop()
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
    list <-  function(filter = '{}', options = '{}'){
      mongo_gridfs_list(fs, filter, options)
    }
    upload <- function(path, name = basename(path), content_type = NULL){
      mongo_gridfs_upload(fs, name, path, content_type)
    }
    download <- function(name, path = name){
      mongo_gridfs_download(fs, name, path)
    }
    read <- function(name){
      mongo_gridfs_read(fs, name)
    }
    write <- function(name, data, content_type = NULL){
      mongo_gridfs_write(fs, name, data, content_type)
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

#' @useDynLib mongolite R_mongo_gridfs_list
mongo_gridfs_list <- function(fs, filter, opts){
  out <- .Call(R_mongo_gridfs_list, fs, bson_or_json(filter), bson_or_json(opts))
  out <- lapply(out, unlist, recursive = TRUE)
  names(out) <- c('id', 'name', 'size', 'date', 'type')
  out$date <- structure(out$date / 1000, class = c("POSIXct", "POSIXt"))
  data.frame(out, stringsAsFactors = FALSE)
}

#' @useDynLib mongolite R_mongo_gridfs_upload
mongo_gridfs_upload <- function(fs, name, path, type){
  stopifnot(is.character(name))
  path <- normalizePath(path, mustWork = TRUE)
  stopifnot(length(name) == length(path))
  id <- rep(NA, length(name))
  if(is.null(type))
    type <- mime::guess_type(name, unknown = NA, empty = NA)
  type <- as.character(rep_len(type, length(name)))
  for(i in seq_along(name)){
    out <- .Call(R_mongo_gridfs_upload, fs, name[i], path[i], type[i])
    id[i] = out$id
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
    out <- .Call(R_mongo_gridfs_download, fs, name[i], path[i])
  }
  structure(out, names = name)
}

#' @useDynLib mongolite R_mongo_gridfs_write
mongo_gridfs_write <- function(fs, name, data, type){
  stopifnot(is.raw(data))
  stopifnot(is.character(name))
  .Call(R_mongo_gridfs_write, fs, name, data, type)
}

#' @useDynLib mongolite R_mongo_gridfs_read
mongo_gridfs_read <- function(fs, name){
  .Call(R_mongo_gridfs_read, fs, name)
}

#' @useDynLib mongolite R_mongo_gridfs_remove
mongo_gridfs_remove <- function(fs, name){
  vapply(name, function(x){
    .Call(R_mongo_gridfs_remove, fs, x)$id
  }, character(1))
}
