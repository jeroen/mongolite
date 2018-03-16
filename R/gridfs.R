#' GridFS API
#'
#' Connect to a GridFS database.
#'
#' @inheritParams mongo
#' @export
#' @param prefix string to prefix the collection name
gridfs <- function(prefix = "fs", db = "test", url = "mongodb://localhost", verbose = FALSE, options = ssl_options()){
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
  fs_object(fs, client, verbose, orig)
}

fs_object <- function(fs, client, verbose, orig){
  self <- local({
    drop <- function(){
      mongo_gridfs_drop(fs)
    }
    list <-  function(filter = '{}', options = '{}'){
      mongo_gridfs_list(fs, filter, options)
    }
    upload <- function(name, path){
      mongo_gridfs_upload(fs, name, path)
    }
    read <- function(name){
      mongo_gridfs_read(fs, name)
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
  rev(as.character(unlist(out, recursive = FALSE)))
}

#' @useDynLib mongolite R_mongo_gridfs_upload
mongo_gridfs_upload <- function(fs, name, path){
  stopifnot(is.character(name))
  path <- normalizePath(path, mustWork = TRUE)
  .Call(R_mongo_gridfs_upload, fs, name, path)
}

#' @useDynLib mongolite R_mongo_gridfs_read
mongo_gridfs_read <- function(fs, name){
  .Call(R_mongo_gridfs_read, fs, name)
}
