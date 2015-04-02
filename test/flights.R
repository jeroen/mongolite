library(nycflights13)
library(mongolite)

m <- mongo_connect(collection = "flights")
if(mongo_collection_count(m) > 0){
  mongo_collection_drop(m)
}

mongo_stream_out(flights, m)
mongo_collection_count(m)
flights2 <- mongo_stream_in(m)
all.equal(as.data.frame(flights), flights2)

