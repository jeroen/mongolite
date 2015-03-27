# install_github("jeroenooms/mongolite")
library(mongolite)

# Init connection
m <- mongo_connect(collection = "diamonds")
try(mongo_collection_drop(m), silent = TRUE)

# Dump some test data
data(diamonds, package="ggplot2")
mongo_write_df(m, diamonds)

# Query
out <- mongo_read_df(m, query = '{"cut" : "Premium", "price" : { "$lt" : 1000 } }')
