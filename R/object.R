#' MongoDB client
#'
#' Initiate a new MongoDB collection.
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
#'   \item{\code{drop()}}{Delete entire collection and all data.}
#'   \item{\code{find(query = '{}', fields = '{"_id" : 0}', skip = 0, limit = 0, handler = NULL, pagesize = 1000, verbose = TRUE)}}{Retrieve 'fields' from all records matching a query. If handler = NULL, data will returned as a single data frame.}
#'   \item{\code{index(add = NULL, remove = NULL)}}{find, add or remove indexes from the collection. Always returns a data frame with current indexes.}
#'   \item{\code{info()}}{Returns collection statistics and server info (if available).}
#'   \item{\code{insert(data, pagesize = 1000, verbose = TRUE)}}{Insert a data frame into the collection.}
#'   \item{\code{remove(query = "{}", multiple = TRUE)}}{Remove record(s) from the collection.}
#'   \item{\code{rename(name, db = "test")}}{Change the name or database of a collection. Changing name is cheap, but changing database is expensive.}
#' }
mongo <- function(collection = "test",  db = "test", url = "mongodb://localhost"){
  con <- mongo_collection_new(url, db, collection)
  mongo_object(con)
}

mongo_object <- function(con){
  client <- attr(con, "client")
  self <- local({
    insert <- function(data, pagesize = 1000, verbose = TRUE)
      mongo_stream_out(data, con, pagesize = pagesize, verbose = verbose)

    find <- function(query = '{}', fields = '{"_id" : 0}', skip = 0, limit = 0, handler = NULL, pagesize = 1000, verbose = TRUE){
      cur <- mongo_collection_find(con, query = query, fields = fields, skip = skip, limit = limit)
      mongo_stream_in(cur, handler = handler, pagesize = pagesize, verbose = verbose)
    }

    aggregate <- function(pipeline = '{}', handler = NULL, pagesize = 1000, verbose = TRUE){
      cur <- mongo_collection_aggregate(con, pipeline)
      mongo_stream_in(cur, handler = handler, pagesize = pagesize, verbose = verbose)
    }

    count <- function(query = '{}')
      mongo_collection_count(con, query)

    remove <- function(query, multiple = FALSE)
      mongo_collection_remove(con, query, multiple)

    drop <- function()
      mongo_collection_drop(con)

    update <- function(query, update = '{"$set":{}}', upsert = FALSE, multiple = FALSE)
      mongo_collection_update(con, query, update, upsert, multiple)

    info <- function(){
      list(
        name = mongo_collection_name(con),
        stats = tryCatch(mongo_collection_stats(con), error = function(e) NULL),
        server = mongo_client_server_status(client)
      )
    }

    rename <- function(name, db = NULL)
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
  print.jeroen(x, title = paste0("<Mongo collection> '", mongo_collection_name(parent.env(x)$con), "'"))
}
