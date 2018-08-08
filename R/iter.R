mongo_iterator <- function(cur){
  self <- local({
    one <- function(){
      mongo_cursor_next_page(cur, size = 1)[[1]]
    }
    batch <- function(size = 1000){
      mongo_cursor_next_page(cur, size = size)
    }
    json <- function(size = 1000){
      as.character(mongo_cursor_next_page(cur, size = size, as_json = TRUE))
    }
    page <- function(size = 1000){
      as.data.frame(jsonlite:::simplify(mongo_cursor_next_page(cur, size = size)))
    }
    environment()
  })
  lockEnvironment(self, TRUE)
  structure(self, class=c("mongo_iter", "jeroen", class(self)))
}

#' @export
print.mongo_iter <- function(x, ...){
  print.jeroen(x, title = paste0("<Mongo iterator>"))
}
