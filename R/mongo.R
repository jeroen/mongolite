#' @useDynLib mongolite R_mongo_connect
mongo_connect <- function(uri = "mongodb://localhost", db = "test", collection = "test"){
  .Call(R_mongo_connect, uri, db, collection)
}

#' @useDynLib mongolite R_mongo_collection_drop
mongo_collection_drop <- function(col){
  .Call(R_mongo_collection_drop, col)
}

#' @useDynLib mongolite R_mongo_collection_name
mongo_collection_name <- function(col){
  .Call(R_mongo_collection_name, col)
}

#' @useDynLib mongolite R_mongo_collection_count
mongo_collection_count <- function(col, query = "{}"){
  .Call(R_mongo_collection_count, col, query)
}

#' @useDynLib mongolite R_mongo_collection_insert
mongo_collection_insert <- function(col, doc, stop_on_error = TRUE){
  .Call(R_mongo_collection_insert, col, doc, stop_on_error)
}

#' @useDynLib mongolite R_mongo_collection_remove
mongo_collection_remove <- function(col, doc, all = TRUE){
  .Call(R_mongo_collection_remove, col, doc, all)
}
