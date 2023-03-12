# Connection Pool
#
# Reuse active connections for multiple collections. We use weak references to
# make sure the connections go out of scope and get collected when no longer used.
new_client <- local({
  pool <- new.env()
  function(params){
    hash <- as.character(openssl::sha1(serialize(params, NULL)))
    client <- get_weakref(pool[[hash]])
    if(!length(client) || null_ptr(client)){
      # Make sure 'client' remains in scope after creating weakref!
      client <- do.call(mongo_client_new, params)
      pool[[hash]] <- make_weakref(client)
    }
    return(client)
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
