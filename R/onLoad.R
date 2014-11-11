#' @useDynLib mongolite R_mongo_init
.onLoad <- function(libname, pkgname){
  .Call(R_mongo_init)
}

#' @useDynLib mongolite R_mongo_cleanup
.onUnload <- function(libpath){
  .Call(R_mongo_cleanup)
}