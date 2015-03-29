# install_github("jeroenooms/mongolite")
library(mongolite)

# Init connection
m <- mongo_connect(collection = "diamonds")
if(mongo_collection_count(m) > 0){
  mongo_collection_drop(m)
}

# Dump some test data
data(diamonds, package="ggplot2")
system.time(mongo_write_df(m, diamonds))

# Query
system.time(out <- mongo_read_df(m, query = '{"cut" : "Premium", "price" : { "$lt" : 1000 } }'))
