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
try(mongo_collection_drop(m), silent = TRUE)

# Insert test data
data(diamonds, package="ggplot2")
mongo_write_df(m, diamonds)

# Perform a query and retrieve data
out <- mongo_read_df(m, query = '{"cut" : "Premium", "price" : { "$lt" : 1000 } }')

# Compare
nrow(out)
nrow(nrow(subset(diamonds, cut == "Premium" & price < 1000)))
```
