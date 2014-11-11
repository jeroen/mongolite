#' @useDynLib mongolite R_mongo_init
.onLoad <- function(pkg, lib){
  .Call(R_mongo_init)
}
