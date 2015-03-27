library(ggplot2)
library(mongolite)

m <- mongo_connect(collection = "diamonds")
mongo_collection_drop(m)
mongo_write_df(m, diamonds)
out <- mongo_read_df(m)

nrow(out)
nrow(diamonds)
