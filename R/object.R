#' MongoDB client
#'
#' Initiate a new MongoDB client.
#'
#' @export
#' @param url address of the mongodb server
#' @param db name of database
#' @param collection name of collection
#' @examples \dontrun{dplyr example
#' library(nycflights13)
#'
#' # Insert some data
#' con <- mongo(collection = "myflights")
#' if(con$count() > 0) con$drop()
#' con$insert(flights)
#'
#' # Basic operations
#' con$count('{"month":1, "day":1}')
#' mydata <- con$find(query = '{"month":1, "day":1}')
#' }
mongo <- function(url = "mongodb://localhost", db = "test", collection = "test"){
  con <- mongo_connect(url, db, collection)
  mongo_object(con)
}

mongo_object <- function(con){
  insert <- function(data, pagesize = 1000, verbose = TRUE, ...){
    mongo_stream_out(data, con, pagesize = pagesize, verbose = verbose, ...)
  }
  find <- function(query = '{}', handler = NULL, pagesize = 1000, verbose = TRUE){
    mongo_stream_in(con, handler = handler, pagesize = pagesize, verbose = verbose, query = query)
  }
  count <- function(query = '{}'){
    mongo_collection_count(con, query)
  }
  drop <- function(){
    mongo_collection_drop(con)
  }
  name <- function(){
    mongo_collection_name(con)
  }
  command <- function(command = '{}'){
    mongo_collection_command(con, command = command)
  }
  environment()
}
