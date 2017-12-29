#' Mongo Options
#'
#' Get and set global client options. Calling with `NULL` parameters returns current
#' values without modifying.
#'
#' Setting [log_level](http://mongoc.org/libmongoc/current/logging.html) to 0 suppresses
#' critical warnings and messages, while 6 is  most verbose and displays all debugging
#' information. Possible values for level are:
#'
#'  - `0`: *error*
#'  - `1`: *critical*
#'  - `2`: *warning*
#'  - `3`: *message*
#'  - `4`: *info* (__default__)
#'  - `5`: *debug*
#'  - `6`: *trace*
#'
#' Note that setting it below 2 will suppress important warnings and setting
#' below 1 will suppress critical errors (not recommended). The default is 4.
#'
#' @export
#' @param log_level integer between 0 and 6 or `NULL` to leave unchanged.
#' @param bigint_as_char logical: parse int64 as strings instead of double.
#' @param date_as_char logical: parse UTC datetime as strings instead of POSIXct.
mongo_options <- function(log_level = NULL, bigint_as_char = NULL, date_as_char = NULL){
  list (
    log_level = mongo_log_level(log_level),
    bigint_as_char = mongo_bigint_as_char(bigint_as_char),
    date_as_char = mongo_date_as_char(date_as_char)
  )
}

#' @useDynLib mongolite R_bigint_as_char
mongo_bigint_as_char <- function(x = NULL){
  .Call(R_bigint_as_char, x)
}

#' @useDynLib mongolite R_date_as_char
mongo_date_as_char <- function(x = NULL){
  .Call(R_date_as_char, x)
}

#' @useDynLib mongolite R_mongo_log_level
mongo_log_level <- function(level = NULL){
  if(!is.null(level)){
    level <- as.integer(level)
    stopifnot(length(level) == 1)
    stopifnot(level >= 0 && level <= 6)
  }
  .Call(R_mongo_log_level, level)
}
