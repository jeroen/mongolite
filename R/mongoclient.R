#' MongoDB client
#'
#' Connect to a MongoDB database. Returns a [mongo_client] connection object with
#' methods listed below.
#'
#' @export
#' @aliases mongolite
#' @references [Mongolite User Manual](https://jeroen.github.io/mongolite/)
#' @param url address of the mongodb server in mongo connection string
#' [URI format](http://docs.mongodb.org/manual/reference/connection-string)
#' @param verbose emit some more output
#' @param options additional connection options such as SSL keys/certs.
#' @return Upon success returns a pointer to database connection instance.
#' @examples # Connect to mongolabs
#' con <- mongo_client(url = "mongodb://readwrite:test@ds043942.mongolab.com:43942/jeroen_test")
#' con$use('testcollection', 'testdatabase')
#' con$close()
#' @section Methods:
#' \describe{
#'   \item{\code{use(collection, db)}}{Creates a [mongo] object representing a collection in a database with the active connection}
#'   \item{\code{server_status()}}{Returns the server status.}
#' }
#' @references Jeroen Ooms (2014). The \code{jsonlite} Package: A Practical and Consistent Mapping Between JSON Data and \R{} Objects. \emph{arXiv:1403.2805}. \url{http://arxiv.org/abs/1403.2805}
mongo_client <- function(url = "mongodb://localhost", verbose = FALSE, options = ssl_options()){
  client <- do.call(mongo_client_new, c(list(uri = url), options))

  # Check if the ptr has died and automatically recreate it
  check_conn <- function() {
    if(null_ptr(client)){
      message("Connection lost. Trying to reconnect with mongo...")
      if(length(options$pem_file) && !file.exists(options$pem_file)){
        options$pem_file <- tempfile()
        writeLines(attr(orig, "pemdata"), orig$options$pem_file) # FIXME
      }
      client <<- do.call(mongo_client_new, c(list(uri = url), options))
      FALSE
    } else TRUE
  }

  # If reconnected, recreates the collection pointer
  check_col <- function(col, col_name, db) {
    if( !check_conn() ) {
      newcol <- mongo_collection_new(client, col_name, db)
      mongo_collection_command_simple(newcol, '{"ping":1}')
      newcol
    } else col
  }

  self <- local({
    use <- function(collection, db){
      connect_to_db_and_coll(db, collection)
    }

    server_status <- function() {
      mongo_client_server_status(client)
    }

    environment()
  })
  lockEnvironment(self, TRUE)

  connect_to_db_and_coll <- function(db, collection) {

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

    mongo_object(col, self, db, collection, verbose, options)
  }

  structure(self, class=c("mongo_client", "jeroen", class(self)))
}
