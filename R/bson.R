json_to_bson <- function(json = "{}"){
  .Call(R_json_to_bson, json)
}

bson_to_json <- function(bson){
  .Call(R_bson_to_json, bson)
}

bson_to_raw <- function(bson){
  .Call(R_bson_to_raw, bson)
}

raw_to_bson <- function(bson){
  .Call(R_raw_to_bson, bson)
}

as.character.bson <- function(x, ...){
  bson_to_json(x)
}

print.bson <- function(x, ...){
  cat(as.character(x));
}
