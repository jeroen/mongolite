# install_github("jeroenooms/mongolite")
library(mongolite)

# Init connection
m <- mongo_connect(collection = "errortest")
if(mongo_collection_count(m) > 0){
  mongo_collection_drop(m)
}

# Some data
mongo_collection_insert_page(m, c('{"foo" : 123}', '{"_id": 123}', '{"foo" : 123}'))

# Dupe key error: inserts 1 records
mongo_collection_insert_page(m, c('{"foo" : 123}', '{"_id": 123}', '{"foo" : 123}'))

# Dupe key error: inserts 2 records
mongo_collection_insert_page(m, c('{"foo" : 123}', '{"_id": 123}', '{"foo" : 123}'), ordered = TRUE)

