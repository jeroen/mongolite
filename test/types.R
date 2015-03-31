library(mongolite)
m <- mongo_connect(collection = "typetest")

mydf <- data.frame(
  int = as.integer(c(1L, 2L, NA)),
  logical = c(T, F, NA),
  real = c(pi, 0, NA),
  date = Sys.Date() + c(1, NA, 3),
  time = Sys.time() + c(1, NA, 3)
)
mydf$raw <- list(charToRaw("foobar"), NULL, charToRaw("bla"))

# Stream from url into mongo
if(mongo_collection_count(m) > 0){
  mongo_collection_drop(m)
}

mongo_stream_out(mydf, m)
mongo_collection_count(m)
out <- mongo_stream_in(m)
