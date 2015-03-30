library(jsonlite)
library(mongolite)

# Stream from url into mongo
m <- mongo_connect(collection = "zips")
if(mongo_collection_count(m) > 0){
  mongo_collection_drop(m)
}

stream_in(url("http://media.mongodb.org/zips.json"), handler = function(df){
  mongo_stream_out(df, m, verbose = FALSE)
})

# Check count
mongo_collection_count(m)

# Import
zips <- mongo_stream_in(m)
