# Run 'mongod' in the console to start MongoDB
# install_github("jeroenooms/mongolite")
library(mongolite)

# Init connection
m <- mongo_connect(collection = "diamonds")
if(mongo_collection_count(m) > 0){
  mongo_collection_drop(m)
}

# Dump some test data
data(diamonds, package="ggplot2")
mongo_stream_out(diamonds, m)

# Query
out <- mongo_stream_in(m)
