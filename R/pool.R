# Connection Pool
#
# Reuse active connections for multiple collections. We use weak references to
# make sure the connections go out of scope and get collected when no longer used.
new_client <- local({
  pool <- new.env()
  function(params){
    hash <- as.character(openssl::md5(serialize(params, NULL)))
    client <- get_weakref(pool[[hash]])
    if(!length(client) || null_ptr(client)){
      pool[[hash]] <- make_weakref(do.call(mongo_client_new, params))
    }
    get_weakref(pool[[hash]])
  }
})


#' @useDynLib mongolite R_make_weakref
make_weakref <- function(x){
  .Call(R_make_weakref, x)
}

#' @useDynLib mongolite R_get_weakref
get_weakref <- function(x){
  .Call(R_get_weakref, x)
}
