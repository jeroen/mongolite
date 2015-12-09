#' MongoDB client
#'
#' Connect to a MongoDB collection. Returns a \code{mongo} connection object with
#' methods listed below.
#'
#' @export
#' @aliases mongolite
#' @param url address of the mongodb server in mongo connection string
#' \href{http://docs.mongodb.org/manual/reference/connection-string/}{URI format}.
#' @param db name of database
#' @param collection name of collection
#' @param verbose emit some more output
#' @return Upon success returns a pointer to a collection on the server.
#' The collection can be interfaced using the methods described below.
#' @examples # Connect to mongolabs
#' con <- mongo("mtcars", url = "mongodb://readwrite:test@ds043942.mongolab.com:43942/jeroen_test")
#' if(con$count() > 0) con$drop()
#' con$insert(mtcars)
#' stopifnot(con$count() == nrow(mtcars))
#'
#' # Query data
#' mydata <- con$find()
#' stopifnot(all.equal(mydata, mtcars))
#' con$drop()
#'
#' # Automatically disconnect when connection is removed
#' rm(con)
#' gc()
#'
#' \dontrun{
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
#' # Stream jsonlines into a connection
#' tmp <- tempfile()
#' m$export(file(tmp))
#'
#' # Remove the collection
#' m$drop()
#'
#' # Import from jsonlines stream from connection
#' dmd <- mongo("diamonds")
#' dmd$import(url("http://jeroenooms.github.io/data/diamonds.json"))
#' dmd$count()
#'
#' # Export
#' dmd$drop()
#' }
#' @section Methods:
#' \describe{
#'   \item{\code{aggregate(pipeline = '{}', handler = NULL, pagesize = 1000)}}{Execute a pipeline using the Mongo aggregation framework.}
#'   \item{\code{count(query = '{}')}}{Count the number of records matching a given \code{query}. Default counts all records in collection.}
#'   \item{\code{distinct(key, query = '{}')}}{List unique values of a field given a particular query.}
#'   \item{\code{drop()}}{Delete entire collection with all data and metadata.}
#'   \item{\code{export(con = stdout(), bson = FALSE)}}{Streams all data from collection to a \code{\link{connection}} in \href{http://ndjson.org}{jsonlines} format (similar to \href{http://docs.mongodb.org/v2.6/reference/mongoexport/}{mongoexport}). Alternatively when \code{bson = TRUE} it outputs the binary \href{http://bsonspec.org/faq.html}{bson} format (similar to \href{http://docs.mongodb.org/manual/reference/program/mongodump/}{mongodump}).}
#'   \item{\code{find(query = '{}', fields = '{"_id" : 0}', sort = '{}', skip = 0, limit = 0, handler = NULL, pagesize = 1000)}}{Retrieve \code{fields} from records matching \code{query}. Default \code{handler} will return all data as a single dataframe.}
#'   \item{\code{import(con, bson = FALSE)}}{Stream import data in \href{http://ndjson.org}{jsonlines} format from a \code{\link{connection}}, similar to the \href{http://docs.mongodb.org/v2.6/reference/mongoimport/}{mongoimport} utility. Alternatively when \code{bson = TRUE} it assumes the binary \href{http://bsonspec.org/faq.html}{bson} format (similar to \href{http://docs.mongodb.org/manual/reference/program/mongorestore/}{mongorestore}).}
#'   \item{\code{index(add = NULL, remove = NULL)}}{List, add, or remove indexes from the collection. The \code{add} and \code{remove} arguments can either be a field name or json object. Returns a dataframe with current indexes.}
#'   \item{\code{info()}}{Returns collection statistics and server info (if available).}
#'   \item{\code{insert(data, pagesize = 1000)}}{Insert a dataframe into the collection.}
#'   \item{\code{iterate(query = '{}', fields = '{"_id":0}', sort = '{}', skip = 0, limit = 0)}}{Runs query and returns iterator to read single records one-by-one.}
#'   \item{\code{mapreduce(map, reduce, query = '{}', sort = '{}', limit = 0, out = NULL, scope = NULL)}}{Performs a map reduce query. The \code{map} and \code{reduce} arguments are strings containing a JavaScript function. Set \code{out} to a string to store results in a collection instead of returning.}
#'   \item{\code{remove(query = "{}", multiple = FALSE)}}{Remove record(s) matching \code{query} from the collection.}
#'   \item{\code{rename(name, db = NULL)}}{Change the name or database of a collection. Changing name is cheap, changing database is expensive.}
#'   \item{\code{update(query, update = '{"$set":{}}', upsert = FALSE, multiple = FALSE)}}{Replace or modify matching record(s) with value of the \code{update} argument.}
#' }
#' @references Jeroen Ooms (2014). The \code{jsonlite} Package: A Practical and Consistent Mapping Between JSON Data and \R{} Objects. \emph{arXiv:1403.2805}. \url{http://arxiv.org/abs/1403.2805}
mongo <- function(collection = "test", db = "test", url = "mongodb://localhost", verbose = TRUE){
  client <- mongo_client_new(url)

  # workaround for missing 'mongoc_client_get_default_database'
  if(missing(db) || is.null(db)){
    if(!is.null(url_db <- mongo_get_default_database(client)))
      db <- url_db
  }

  col <- mongo_collection_new(client, collection, db)
  mongo_collection_command_simple(col, '{"ping":1}')
  mongo_object(col, client, verbose = verbose)
}

mongo_object <- function(col, client, verbose){
  self <- local({
    insert <- function(data, pagesize = 1000)
      mongo_stream_out(data, col, pagesize = pagesize, verbose = verbose)

    find <- function(query = '{}', fields = '{"_id":0}', sort = '{}', skip = 0, limit = 0, handler = NULL, pagesize = 1000){
      cur <- mongo_collection_find(col, query = query, sort = sort, fields = fields, skip = skip, limit = limit)
      mongo_stream_in(cur, handler = handler, pagesize = pagesize, verbose = verbose)
    }

    iterate <- function(query = '{}', fields = '{"_id":0}', sort = '{}', skip = 0, limit = 0) {
      cur <- mongo_collection_find(col, query = query, sort = sort, fields = fields, skip = skip, limit = limit)
      # make sure 'col' does not go out of scope to prevent gc
      mongo_iterator(cur, col)
    }

    export <- function(con = stdout(), bson = FALSE){
      if(isTRUE(bson)){
        mongo_dump(col, con, verbose = verbose)
      } else {
        mongo_export(col, con, verbose = verbose)
      }
    }

    import <- function(con, bson = FALSE){
      if(isTRUE(bson)){
        mongo_restore(col, con, verbose = verbose)
      } else {
        mongo_import(col, con, verbose = verbose)
      }
    }

    aggregate <- function(pipeline = '{}', handler = NULL, pagesize = 1000){
      cur <- mongo_collection_aggregate(col, pipeline)
      mongo_stream_in(cur, handler = handler, pagesize = pagesize, verbose = verbose)
    }

    count <- function(query = '{}')
      mongo_collection_count(col, query)

    remove <- function(query, multiple = FALSE)
      mongo_collection_remove(col, query, multiple)

    drop <- function()
      mongo_collection_drop(col)

    update <- function(query, update = '{"$set":{}}', upsert = FALSE, multiple = FALSE)
      mongo_collection_update(col, query, update, upsert, multiple)

    mapreduce <- function(map, reduce, query = '{}', sort = '{}', limit = 0, out = NULL, scope = NULL){
      cur <- mongo_collection_mapreduce(col, map = map, reduce = reduce, query = query,
        sort = sort, limit = limit, out = out, scope = scope)
      results <- mongo_stream_in(cur, verbose = FALSE)
      if(is.null(out))
        results[[1, "results"]]
      else
        results
    }

    distinct <- function(key, query = '{}'){
      out <- mongo_collection_distinct(col, key, query)
      jsonlite:::simplify(out$values)
    }

    info <- function(){
      list(
        name = mongo_collection_name(col),
        stats = tryCatch(mongo_collection_stats(col), error = function(e) NULL),
        server = mongo_client_server_status(client)
      )
    }

    rename <- function(name, db = NULL)
      mongo_collection_rename(col, db, name)

    index <- function(add = NULL, remove = NULL){
      if(length(add))
        mongo_collection_create_index(col, add);

      if(length(remove))
        mongo_collection_drop_index(col, remove);

      mongo_collection_find_indexes(col)
    }
    environment()
  })
  lockEnvironment(self, TRUE)
  structure(self, class=c("mongo", "jeroen", class(self)))
}

#' @export
print.mongo <- function(x, ...){
  print.jeroen(x, title = paste0("<Mongo collection> '", mongo_collection_name(parent.env(x)$col), "'"))
}
