#' @useDynLib mongolite C_join_pairlist
join_pairlist <- function(x, y){
  .Call(C_join_pairlist, x, y)
}
