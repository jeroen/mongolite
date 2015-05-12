#' MongoDB client
#'
#' Connect to a MongoDB collection.
#'
#' @export
#' @param url address of the mongodb server
#' @param db name of database
#' @param collection name of collection
#' @return Upon success returns a pointer to a collection on the server.
#' The collection can be interfaced using the methods described below.
#' @examples \dontrun{
#' # dplyr example
#' library(nycflights13)
#'
#' # Insert some data
#' m <- mongo(collection = "nycflights")
#' m$drop()
#' m$insert(flights)
#'
#' # Basic queries
#' m$count('{"month":1, "day":1}')
#' jan1 <- m$find('{"month":1, "day":1}')
#'
#' # Sorting
#' jan1 <- m$find('{"month":1,"day":1}', sort='{"distance":-1}')
#' head(jan1)
#'
#' # Sorting on large data requires index
#' m$index(add = "distance")
#' allflights <- m$find(sort='{"distance":-1}')
#'
#' # Select columns
#' jan1 <- m$find('{"month":1,"day":1}', fields = '{"_id":0, "distance":1, "carrier":1}')
#'
#' # List unique values
#' m$distinct("carrier")
#' m$distinct("carrier", '{"distance":{"$gt":3000}}')
#'
#' # Tabulate
#' m$aggregate('[{"$group":{"_id":"$carrier", "count": {"$sum":1}, "average":{"$avg":"$distance"}}}]')
#'
#' # Map-reduce (binning)
#' hist <- m$mapreduce(
#'   map = "function(){emit(Math.floor(this.distance/100)*100, 1)}",
#'   reduce = "function(id, counts){return Array.sum(counts)}"
#' )
#'
#' # Remove the collection
#' m$drop()
#' }
#' @section Methods:
#' \describe{
#'   \item{\code{aggregate(pipeline = '{}', handler = NULL, pagesize = 1000, verbose = TRUE)}}{Execute a pipeline using the Mongo aggregation framework.}
#'   \item{\code{count(query = '{}')}}{Count the number of records matching a given \code{query}. Default counts all records in collection.}
#'   \item{\code{distinct(key, query = '{}')}}{List unique values of a field given a particular query.}
#'   \item{\code{drop()}}{Delete entire collection with all data and metadata.}
#'   \item{\code{find(query = '{}', fields = '{"_id" : 0}', skip = 0, limit = 0, handler = NULL, pagesize = 1000, verbose = TRUE)}}{Retrieve \code{fields} from records matching \code{query}. Default \code{handler} will return all data as a single dataframe.}
#'   \item{\code{index(add = NULL, remove = NULL)}}{List, add, or remove indexes from the collection. The \code{add} and \code{remove} arguments can either be a field name or json object. Returns a dataframe with current indexes.}
#'   \item{\code{info()}}{Returns collection statistics and server info (if available).}
#'   \item{\code{insert(data, pagesize = 1000, verbose = TRUE)}}{Insert a dataframe into the collection.}
#'   \item{\code{mapreduce(map, reduce)}}{Performs a map reduce query. The \code{map} and \code{reduce} arguments are strings containing a JavaScript function.}
#'   \item{\code{remove(query = "{}", multiple = FALSE)}}{Remove record(s) matching \code{query} from the collection.}
#'   \item{\code{rename(name, db = "test")}}{Change the name or database of a collection. Changing name is cheap, changing database is expensive.}
#'   \item{\code{update(query, update = '{"$set":{}}', upsert = FALSE, multiple = FALSE)}}{Replace or modify matching record(s) with value of the \code{update} argument.}
#' }
#' @references Jeroen Ooms (2014). The \code{jsonlite} Package: A Practical and Consistent Mapping Between JSON Data and \R{} Objects. \emph{arXiv:1403.2805}. \url{http://arxiv.org/abs/1403.2805}
mongo <- function(collection = "test",  db = "test", url = "mongodb://localhost"){
  client <- mongo_client_new(url)
  con <- mongo_collection_new(client, collection, db)
  mongo_object(con, client)
}

mongo_object <- function(con, client){
  self <- local({
    insert <- function(data, pagesize = 1000, verbose = TRUE)
      mongo_stream_out(data, con, pagesize = pagesize, verbose = verbose)

    find <- function(query = '{}', fields = '{"_id":0}', sort = '{"_id":1}', skip = 0, limit = 0, handler = NULL, pagesize = 1000, verbose = TRUE){
      cur <- mongo_collection_find(con, query = query, sort = sort, fields = fields, skip = skip, limit = limit)
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

    mapreduce <- function(map, reduce){
      cur <- mongo_collection_mapreduce(con, map, reduce)
      out <- mongo_stream_in(cur, verbose = FALSE)
      out[[1, "results"]]
    }

    distinct <- function(key, query = '{}'){
      out <- mongo_collection_distinct(con, key, query)
      jsonlite:::simplify(out$values)
    }

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
