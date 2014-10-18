json_to_bson <- function(json = "{}"){
  .Call(R_json_to_bson, json)
}

bson_to_json <- function(bson){
  .Call(R_bson_to_json, bson)
}

print.bson <- function(x, ...){
  cat(bson_to_json(x));
}