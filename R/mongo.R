mongo_connect <- function(uri = "mongodb://localhost", db = "test", collection = "test"){
  .Call(R_mongo_connect, uri, db, collection)
}
