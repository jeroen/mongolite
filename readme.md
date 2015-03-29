Mongolite
=========

Streaming Mongo Client for R.

[![Build Status](https://travis-ci.org/jeroenooms/mongolite.svg?branch=master)](https://travis-ci.org/jeroenooms/mongolite)

Hello World
-----------

```r
# devtools::install_github("jeroenooms/mongolite")
library(mongolite)

# Initiate connection to local mongod
m <- mongo_connect(collection = "diamonds")

# Drop existing data if any
if(mongo_collection_count(m) > 0){
  mongo_collection_drop(m)
}

# Insert test data
data(diamonds, package="ggplot2")
mongo_write_df(m, diamonds)

# Check records
mongo_collection_count(m)
nrow(diamonds)

# Perform a query and retrieve data
out <- mongo_read_df(m, query = '{"cut" : "Premium", "price" : { "$lt" : 1000 } }')

# Compare
nrow(out)
nrow(nrow(subset(diamonds, cut == "Premium" & price < 1000)))
```
