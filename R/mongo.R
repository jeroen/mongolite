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

#' @useDynLib mongolite R_mongo_collection_rename
mongo_collection_rename <- function(col, db = "test", name){
  .Call(R_mongo_collection_rename, col, db, name)
}

#' @useDynLib mongolite R_mongo_collection_count
mongo_collection_count <- function(col, query = "{}"){
  .Call(R_mongo_collection_count, col, bson_or_json(query))
}

#' @useDynLib mongolite R_mongo_collection_command
mongo_collection_command <- function(col, command = "{}"){
  .Call(R_mongo_collection_command, col, bson_or_json(command))
}

#' @useDynLib mongolite R_mongo_collection_insert_bson
mongo_collection_insert_bson <- function(col, doc, stop_on_error = TRUE){
  .Call(R_mongo_collection_insert_bson, col, bson_or_json(doc), stop_on_error)
}

#' @useDynLib mongolite R_mongo_collection_insert_page
mongo_collection_insert_page <- function(col, json, stop_on_error = TRUE){
  .Call(R_mongo_collection_insert_page, col, json, stop_on_error)
}

#' @useDynLib mongolite R_mongo_collection_remove
mongo_collection_remove <- function(col, doc, multiple = TRUE){
  .Call(R_mongo_collection_remove, col, bson_or_json(doc), multiple)
}

#' @useDynLib mongolite R_mongo_collection_find
mongo_collection_find <- function(col, query = '{}', fields = '{"_id" : 0}', skip = 0, limit = 0){
  .Call(R_mongo_collection_find, col, bson_or_json(query), bson_or_json(fields), skip, limit)
}

#' @useDynLib mongolite R_mongo_cursor_more
mongo_cursor_more <- function(cursor){
  .Call(R_mongo_cursor_more, cursor)
}

#' @useDynLib mongolite R_mongo_collection_create_index
mongo_collection_create_index <- function(col, doc = '{}'){
  .Call(R_mongo_collection_create_index, col, bson_or_json(doc))
}

#' @useDynLib mongolite R_mongo_collection_drop_index
mongo_collection_drop_index <- function(col, name){
  .Call(R_mongo_collection_drop_index, col, name)
}

#' @useDynLib mongolite R_mongo_cursor_next_bson
mongo_cursor_next_bson <- function(cursor){
  .Call(R_mongo_cursor_next_bson, cursor)
}

#' @useDynLib mongolite R_mongo_cursor_next_page
mongo_cursor_next_page <- function(cursor, size = 100){
  .Call(R_mongo_cursor_next_page, cursor, size = size)
}

#' @useDynLib mongolite R_mongo_collection_find_indexes
mongo_collection_find_indexes <- function(col){
  cur <- .Call(R_mongo_collection_find_indexes, col)
  out <- mongo_cursor_next_page(cur)
  out <- Filter(length, out)
  jsonlite:::simplify(out)
}
