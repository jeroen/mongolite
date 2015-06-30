# A poor man's oo system.

#' @export
print.jeroen <- function(x, title = paste0("<", class(x)[1], ">"), ...){
  ns <- ls(x)
  cat(title, "\n")
  lapply(ns, function(fn){
    cat(format_function(x[[fn]], fn), sep = "\n")
  })
  invisible()
}

#' @export
`$.jeroen` <- function(x, y){
  if(!exists(y, x, inherits = FALSE)){
    stop("Class '", class(x)[1], "' has no field '", y, "'", call. = FALSE)
  }
  get(y, x, inherits = FALSE)
}

#' @export
names.jeroen <- function(x, ...){
  ls(x, ...)
}

#' @export
`[[.jeroen` <- `$.jeroen`

#' @export
`[.jeroen` <- `$.jeroen`

# Pretty format function headers
format_function <- function(fun, name = deparse(substitute(fun))){
  #header <- sub("\\{$", "", capture.output(fun)[1])
  header <- utils::head(deparse(args(fun), 100L), -1)
  header <- sub("^[ ]*", "   ", header)
  header[1] <- sub("^[ ]*function ?", paste0(" $", name), header[1])
  header
}

# Override default call argument.
stop <- function(..., call. = FALSE){
  base::stop(..., call. = call.)
}
