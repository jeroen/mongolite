#' @useDynLib mongolite R_json_to_bson
json_to_bson <- function(json = "{}"){
  .Call(R_json_to_bson, json)
}

#' @useDynLib mongolite R_bson_to_json
bson_to_json <- function(bson){
  .Call(R_bson_to_json, bson)
}

#' @useDynLib mongolite R_bson_to_raw
bson_to_raw <- function(bson){
  .Call(R_bson_to_raw, bson)
}

#' @useDynLib mongolite R_raw_to_bson
raw_to_bson <- function(bson){
  .Call(R_raw_to_bson, bson)
}

#' @useDynLib mongolite R_bson_to_list
bson_to_list <- function(bson){
  .Call(R_bson_to_list, bson)
}

#' @export
as.character.bson <- function(x, ...){
  bson_to_json(x)
}

#' @export
print.bson <- function(x, ...){
  cat(as.character(x));
}

setGeneric("serialize")
setOldClass("bson")
setMethod("serialize", "bson", function(object, connection){
  if(!missing(connection)) {
    writeBin(bson_to_raw(object), connection)
  } else {
    bson_to_raw(object);
  }
});