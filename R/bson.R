# High level wrapper
#' @importFrom jsonlite validate
bson_or_json <- function(x, allowNull = FALSE){
  if(inherits(x, "bson")){
    return(x)
  } else if(is.character(x)) {
    if(!validate(x))
      stop("Invalid JSON object: ", substring(x, 1, 200))
    json_to_bson(paste(x, collapse = "\n"))
  } else if(is.null(x) && isTRUE(allowNull)){
    return(NULL)
  } else {
    stop("argument must be bson or json.")
  }
}

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
  cat(paste0("BSON: ", as.character(x), "\n"));
}

#' @export
print.mongo_collection <- function(x, ...){
  cat(paste0("Mongo collection '", mongo_collection_name(x), "'\n"));
}

#' @useDynLib mongolite R_null_ptr
null_ptr <- function(x){
  .Call(R_null_ptr, x)
}

#' Create BSON Object IDs
#'
#' When it is necessary to keep track of BSON OIDs, you may want to create them
#' manually.
#'
#' @param n The number of OIDs to create.
#'
#' @return A vector of class \code{"bson_oid"}.
#'
#' @examples
#' df <- data.frame(`_id` = I(create_bson_oid(10)),
#'                  x = 1:10, check.names = FALSE)
#' \dontrun{
#'
#' # Insert this data into Mongo.
#' m <- mongo("test_oid", verbose = FALSE)
#' m$drop()
#' m$insert(df)
#'
#' # The OIDs of the output should match.
#' m$find(fields = "{}")
#' }
#'
#' @useDynLib mongolite R_create_bson_oid
#' @export
create_bson_oid <- function(n = 1) {
  stopifnot(is.numeric(n) && n >= 1)
  .Call(R_create_bson_oid, n)
}

#' @export
print.bson_oid <- function(x, ...) {
  print(as.character(x))
}

#' @export
`[.bson_oid` <- function(x, i, ...) {
  out <- NextMethod("[")
  class(out) <- class(x)
  out
}

setOldClass("bson_oid")

# Supporting insert() requires implementing asJSON from the jsonlite package.
# Since that generic is not public, this is a workaround.
setGeneric("asJSON", package = "jsonlite")

setMethod("asJSON", "bson_oid", function(x, ...) {
  # Inspired by jsonlite's asJSON.POSIXt.R
  df <- data.frame(`$oid` = unclass(x), check.names = FALSE)
  if (inherits(x, "scalar")) {
    class(df) <- c("scalar", class(df))
  }
  asJSON(df, ...)
})

#setGeneric("serialize")
#setOldClass("bson")
#setMethod("serialize", "bson", function(object, connection){
#  if(!missing(connection)) {
#    writeBin(bson_to_raw(object), connection)
#  } else {
#    bson_to_raw(object);
#  }
#});
