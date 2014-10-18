mongo_connect <- function(uri = "mongodb://localhost", db = "test", collection = "test"){
  .Call(R_mongo_connect, uri, db, collection)
}

mongo_collection_drop <- function(col){
  .Call(R_mongo_collection_drop, col)
}

mongo_collection_name <- function(col){
  .Call(R_mongo_collection_name, col)
}

mongo_collection_count <- function(col, query = "{}"){
  .Call(R_mongo_collection_count, col, query)
}

json_to_bson <- function(json = "{}"){
  .Call(R_json_to_bson, json)
}

