library(nycflights13)
library(mongolite)

m <- mongo_connect(collection = "flights")
mongo_collection_drop(m)
mongo_write_df(m, flights)

