#' Mongo Log Level
#'
#' Get or sets the mongo logging level. Setting to 0 suppresses critical
#' warnings and messages, 6 is most verbose and displays all debugging information.
#' Default is 4 (info). Function always returns the current logging level.
#'
#'
#' The \code{level} parameter sets the \emph{maximum} level of
#' events which will be shown by R.
#' See the \href{http://api.mongodb.org/c/current/logging.html}{mongoc manual}
#' for details. Possible values for level are:
#'
#' \preformatted{
#'  0: error
#'  1: critical
#'  2: warning
#'  3: message
#'  4: info
#'  5: debug
#'  6: trace
#' }
#'
#' Note that setting it below 2 will suppress important warnings and setting
#' below 1 will suppress critical erros (not recommended). The default is 4.
#'
#' @export
#' @param level \code{NULL} to leave unchanged or integer between 0 (error)
#' and 6 (trace) to update global logging level.
#' @useDynLib mongolite R_mongo_log_level
mongo_log_level <- function(level = NULL){
  if(!is.null(level)){
    level <- as.integer(level)
    stopifnot(length(level) == 1)
    stopifnot(level >= 0 && level <= 6)
  }
  .Call(R_mongo_log_level, level)
}
