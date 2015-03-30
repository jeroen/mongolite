library(nycflights13)
library(mongolite)

m <- mongo_connect(collection = "flights")
mongo_stream_out(flights, m)

