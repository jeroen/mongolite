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
#' @section Methods:
#' \describe{
#'   \item{\code{count(query = '{}')}}{Count the number of records for a given query. If no query is specified, counts all records.}
#'   \item{\code{drop()}}{Delete all records from the collection.}
#' }
mongo <- function(collection = "test",  db = "test", url = "mongodb://localhost"){
  con <- mongo_connect(url, db, collection)
  mongo_object(con)
}

mongo_object <- function(con){
  self <- local({
    insert <- function(data, pagesize = 1000, verbose = TRUE, ...)
      mongo_stream_out(data, con, pagesize = pagesize, verbose = verbose, ...)

    find <- function(query = '{}', handler = NULL, pagesize = 1000, verbose = TRUE)
      mongo_stream_in(con, handler = handler, pagesize = pagesize, verbose = verbose, query = query)

    count <- function(query = '{}')
      mongo_collection_count(con, query)

    remove <- function(query = '{}', multiple = TRUE)
      mongo_collection_remove(con, query, multiple)

    drop <- function()
      mongo_collection_drop(con)

    info <- function(){
      list(
        name = mongo_collection_name(con),
        stats = mongo_collection_stats(con)
      )
    }

    rename <- function(name, db = "test")
      mongo_collection_rename(con, db, name)

    index <- function(add = NULL, remove = NULL){
      if(length(add))
        mongo_collection_create_index(con, add);

      if(length(remove))
        mongo_collection_drop_index(con, remove);

      mongo_collection_find_indexes(con)
    }
    environment()
  })
  lockEnvironment(self, TRUE)
  structure(self, class=c("mongo", "jeroen", class(self)))
}

#' @export
print.mongo <- function(x, ...){
  print.jeroen(x, title = paste0("<Mongo collection> '", x$info()$name, "'"))
}
