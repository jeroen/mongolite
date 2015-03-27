library(ggplot2)
library(mongolite)

# Init connection
m <- mongo_connect(collection = "diamonds")
try(mongo_collection_drop(m), silent = TRUE)

# Dump some test data
mongo_write_df(m, diamonds)

# Query
out <- mongo_read_df(m, query = '{"cut" : "Premium", "price" : { "$lt" : 1000 } }')
