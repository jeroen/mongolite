#' MongoDB client
#'
#' Connect to a MongoDB collection. Returns a [mongo] connection object with
#' methods listed below. Connections automatically get pooled between collection
#' and gridfs objects to the same database.
#'
#' This manual page is deliberately minimal, see the
#' [mongolite user manual](https://jeroen.github.io/mongolite/) for more details
#' and worked examples.
#'
#' @export
#' @aliases mongolite
#' @references [Mongolite User Manual](https://jeroen.github.io/mongolite/)
#' @param url address of the mongodb server in mongo connection string
#' [URI format](http://docs.mongodb.org/manual/reference/connection-string)
#' @param db name of database
#' @param collection name of collection
#' @param verbose emit some more output
#' @param options additional connection options such as SSL keys/certs.
#' @return Upon success returns a pointer to a collection on the server.
#' The collection can be interfaced using the methods described below.
#' @examples # Connect to demo server
#' con <- mongo("mtcars", url =
#'   "mongodb://readwrite:test@mongo.opencpu.org:43942/jeroen_test?retryWrites=false")
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
#' \donttest{
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
#' dmd$import(url("http://jeroen.github.io/data/diamonds.json"))
#' dmd$count()
#'
#' # Export
#' dmd$drop()
#' }
#' @section Methods:
#' \describe{
#'   \item{\code{aggregate(pipeline = '{}', handler = NULL, pagesize = 1000, iterate = FALSE)}}{Execute a pipeline using the Mongo aggregation framework. Set \code{iterate = TRUE} to return an iterator instead of data frame.}
#'   \item{\code{count(query = '{}')}}{Count the number of records matching a given \code{query}. Default counts all records in collection.}
#'   \item{\code{disconnect(gc = TRUE)}}{Disconnect collection. The \emph{connection} gets disconnected once the client is not used by collections in the pool.}
#'   \item{\code{distinct(key, query = '{}')}}{List unique values of a field given a particular query.}
#'   \item{\code{drop()}}{Delete entire collection with all data and metadata.}
#'   \item{\code{export(con = stdout(), bson = FALSE, query = '{}', fields = '{}', sort = '{"_id":1}')}}{Streams all data from collection to a \code{\link{connection}} in \href{http://ndjson.org}{jsonlines} format (similar to \href{http://docs.mongodb.org/v2.6/reference/mongoexport/}{mongoexport}). Alternatively when \code{bson = TRUE} it outputs the binary \href{http://bsonspec.org/faq.html}{bson} format (similar to \href{http://docs.mongodb.org/manual/reference/program/mongodump/}{mongodump}).}
#'   \item{\code{find(query = '{}', fields = '{"_id" : 0}', sort = '{}', skip = 0, limit = 0, handler = NULL, pagesize = 1000)}}{Retrieve \code{fields} from records matching \code{query}. Default \code{handler} will return all data as a single dataframe.}
#'   \item{\code{import(con, bson = FALSE)}}{Stream import data in \href{http://ndjson.org}{jsonlines} format from a \code{\link{connection}}, similar to the \href{http://docs.mongodb.org/v2.6/reference/mongoimport/}{mongoimport} utility. Alternatively when \code{bson = TRUE} it assumes the binary \href{http://bsonspec.org/faq.html}{bson} format (similar to \href{http://docs.mongodb.org/manual/reference/program/mongorestore/}{mongorestore}).}
#'   \item{\code{index(add = NULL, remove = NULL)}}{List, add, or remove indexes from the collection. The \code{add} and \code{remove} arguments can either be a field name or json object. Returns a dataframe with current indexes.}
#'   \item{\code{info()}}{Returns collection statistics and server info (if available).}
#'   \item{\code{insert(data, pagesize = 1000, stop_on_error = TRUE, ...)}}{Insert rows into the collection. Argument 'data' must be a data-frame, named list (for single record) or character vector with json strings (one string for each row). For lists and data frames, arguments in \code{...} get passed to \code{\link[jsonlite:toJSON]{jsonlite::toJSON}}}
#'   \item{\code{iterate(query = '{}', fields = '{"_id":0}', sort = '{}', skip = 0, limit = 0)}}{Runs query and returns iterator to read single records one-by-one.}
#'   \item{\code{mapreduce(map, reduce, query = '{}', sort = '{}', limit = 0, out = NULL, scope = NULL)}}{Performs a map reduce query. The \code{map} and \code{reduce} arguments are strings containing a JavaScript function. Set \code{out} to a string to store results in a collection instead of returning.}
#'   \item{\code{remove(query = "{}", just_one = FALSE)}}{Remove record(s) matching \code{query} from the collection.}
#'   \item{\code{rename(name, db = NULL)}}{Change the name or database of a collection. Changing name is cheap, changing database is expensive.}
#'   \item{\code{replace(query, update = '{}', upsert = FALSE)}}{Replace matching record(s) with value of the \code{update} argument.}
#'   \item{\code{run(command = '{"ping": 1}', simplify = TRUE)}}{Run a raw mongodb command on the database. If the command returns data, output is simplified by default, but this can be disabled.}
#'   \item{\code{update(query, update = '{"$set":{}}', upsert = FALSE, multiple = FALSE)}}{Modify fields of matching record(s) with value of the \code{update} argument.}
#' }
#' @references Jeroen Ooms (2014). The \code{jsonlite} Package: A Practical and Consistent Mapping Between JSON Data and \R{} Objects. \emph{arXiv:1403.2805}. \url{http://arxiv.org/abs/1403.2805}
mongo <- function(collection = "test", db = "test", url = "mongodb://localhost", verbose = FALSE, options = ssl_options()){
  client <- new_client(c(list(uri = url), options))

  # workaround for missing 'mongoc_client_get_default_database'
  if(missing(db) || is.null(db)){
    url_db <- mongo_get_default_database(client)
    if(length(url_db) && nchar(url_db))
      db <- url_db
  }

  col <- mongo_collection_new(client, collection, db)
  mongo_collection_command_simple(col, '{"ping":1}')
  orig <- list(
    name = tryCatch(mongo_collection_name(col), error = function(e){collection}),
    db = db,
    url = url,
    options = options
  )
  if(length(options$pem_file) && file.exists(options$pem_file))
    attr(orig, "pemdata") <- readLines(options$pem_file)

  rm(client) #needed for m$disconnect() to work
  mongo_object(col, verbose = verbose, orig)
}

mongo_object <- function(col, verbose, orig){
  # Check if the ptr has died and automatically recreate it
  check_col <- function(){
    if(null_ptr(col)){
      message("Connection lost. Trying to reconnect with mongo...")
      col <<- collection_reset(orig)
    }
  }

  # The reference object
  self <- local({
    insert <- function(data, pagesize = 1000, stop_on_error = TRUE, ...){
      check_col()
      if(is.data.frame(data)){
        mongo_stream_out(data, col, pagesize = pagesize, verbose = verbose, stop_on_error = stop_on_error, ...)
      } else if(is.list(data) && !is.null(names(data))){
        mongo_collection_insert_page(col, mongo_to_json(data, ...), stop_on_error = stop_on_error)
      } else if(is.character(data)) {
        if(!all(is_valid <- vapply(data, jsonlite::validate, logical(1), USE.NAMES = FALSE))){
          el <- paste(which(!is_valid), collapse = ", ")
          stop("Argument 'data' is a character vector but contains invalid JSON at elements: ", el)
        }
        if(!all(is_valid <- grepl("^\\s*\\{", data))){
          el <- paste(which(!is_valid), collapse = ", ")
          stop("Argument 'data' contains strings that are not JSON objects at elements: ", el)
        }
         mongo_collection_insert_page(col, data, stop_on_error = stop_on_error)
      } else if(inherits(data, "bson")){
        mongo_collection_insert_bson(col, data, stop_on_error = stop_on_error)
      } else {
        stop("Argument 'data' must be a data frame, named list, or character vector with json strings")
      }
    }

    find <- function(query = '{}', fields = '{"_id":0}', sort = '{}', skip = 0, limit = 0, handler = NULL, pagesize = 1000){
      check_col()
      cur <- mongo_collection_find(col, query = query, sort = sort, fields = fields, skip = skip, limit = limit)
      mongo_stream_in(cur, handler = handler, pagesize = pagesize, verbose = verbose)
    }

    iterate <- function(query = '{}', fields = '{"_id":0}', sort = '{}', skip = 0, limit = 0) {
      check_col()
      cur <- mongo_collection_find(col, query = query, sort = sort, fields = fields, skip = skip, limit = limit)
      mongo_iterator(cur)
    }

    export <- function(con = stdout(), bson = FALSE, query = '{}', fields = '{}', sort = '{"_id":1}'){
      check_col()
      if(isTRUE(bson)){
        mongo_dump(col, con, verbose = verbose, query = query, fields = fields, sort = sort)
      } else {
        mongo_export(col, con, verbose = verbose, query = query, fields = fields, sort = sort)
      }
    }

    import <- function(con, bson = FALSE){
      check_col()
      if(isTRUE(bson)){
        mongo_restore(col, con, verbose = verbose)
      } else {
        mongo_import(col, con, verbose = verbose)
      }
    }

    aggregate <- function(pipeline = '{}', options = '{"allowDiskUse":true}', handler = NULL,
                          pagesize = 1000, iterate = FALSE){
      check_col()
      cur <- mongo_collection_aggregate(col, pipeline, options)
      if(isTRUE(iterate)){
        mongo_iterator(cur)
      } else {
        mongo_stream_in(cur, handler = handler, pagesize = pagesize, verbose = verbose)
      }
    }

    count <- function(query = '{}'){
      check_col()
      mongo_collection_count(col, query)
    }

    remove <- function(query, just_one = FALSE){
      check_col()
      invisible(mongo_collection_remove(col, query, just_one))
    }

    drop <- function(){
      check_col()
      invisible(mongo_collection_drop(col))
    }

    update <- function(query, update = '{"$set":{}}', filters = NULL, upsert = FALSE, multiple = FALSE){
      check_col()
      mongo_collection_update(col, query, update, filters, upsert, multiple = multiple, replace = FALSE)
    }

    replace <- function(query, update = '{}', upsert = FALSE){
      check_col()
      mongo_collection_update(col, query, update, upsert = upsert, replace = TRUE)
    }

    mapreduce <- function(map, reduce, query = '{}', sort = '{}', limit = 0, out = NULL, scope = NULL){
      check_col()
      cur <- mongo_collection_mapreduce(col, map = map, reduce = reduce, query = query,
        sort = sort, limit = limit, out = out, scope = scope)
      results <- mongo_stream_in(cur, verbose = FALSE)
      if(is.null(out))
        results[[1, "results"]]
      else
        results
    }

    distinct <- function(key, query = '{}'){
      check_col()
      out <- mongo_collection_distinct(col, key, query)
      jsonlite:::simplify(out$values)
    }

    info <- function(){
      check_col()
      client <- ptr_get_prot(col)
      structure(list(
        collection = mongo_collection_name(col),
        db = mongo_get_default_database(client),
        stats = tryCatch(mongo_collection_stats(col), error = function(e) NULL),
        server = mongo_client_server_status(col)
      ), class = "miniprint")
    }

    rename <- function(name, db = NULL){
      check_col()
      out <- mongo_collection_rename(col, db, name)
      orig <<- list(
        name =  tryCatch(mongo_collection_name(col), error = function(e){name}),
        db = ifelse(is.null(db), orig$db, db),
        url = orig$url
      )
      orig
    }

    run <- function(command = '{"ping": 1}', simplify = TRUE){
      mongo_collection_command_simple(col, command, simplify)
    }

    index <- function(add = NULL, remove = NULL){
      check_col()
      if(length(add))
        mongo_collection_create_index(col, add);

      if(length(remove))
        mongo_collection_drop_index(col, remove);

      mongo_collection_find_indexes(col)
    }

    # C driver disconnects when 'client' gets destroyed (which is protected by 'col')
    disconnect <- function(gc = TRUE){
      mongo_collection_disconnect(col)
      if(isTRUE(gc))
        base::gc()
      invisible()
    }
    environment()
  })
  lockEnvironment(self, TRUE)
  structure(self, class=c("mongo", "jeroen", class(self)))
}

collection_reset <- function(orig){
  if(length(orig$options$pem_file) && !file.exists(orig$options$pem_file)){
    orig$options$pem_file <- tempfile()
    writeLines(attr(orig, "pemdata"), orig$options$pem_file)
  }
  client <- new_client(c(list(uri = orig$url), orig$options))
  newcol <- mongo_collection_new(client, orig$name, orig$db)
  mongo_collection_command_simple(newcol, '{"ping":1}')
  newcol
}

#' @export
print.mongo <- function(x, ...){
  parent.env(x)$check_col()
  print.jeroen(x, title = paste0("<Mongo collection> '", mongo_collection_name(parent.env(x)$col), "'"))
}

#' @export
print.miniprint <- function(x, ...){
  utils::str(unclass(x), max.level = 2)
  invisible(x)
}

#setGeneric("serialize")
#setOldClass("jeroen")
#setMethod("serialize", "jeroen", function(object, connection){
#  if(!missing(connection)) {
#    writeBin(bson_to_raw(object), connection)
#  } else {
#    bson_to_raw(object);
#  }
#});
