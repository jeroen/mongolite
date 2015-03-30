Mongolite
=========

Streaming Mongo Client for R.

[![Build Status](https://travis-ci.org/jeroenooms/mongolite.svg?branch=master)](https://travis-ci.org/jeroenooms/mongolite)

Install
-------

On most platforms, starting the MongoDB server is as easy as:

```
mongod
```

To install the R package:

```r
library(devtools)
install_github("jeroenooms/mongolite")
```

Hello World
-----------

```r
library(mongolite)

# Initiate connection to local mongod
m <- mongo_connect(collection = "diamonds")

# Insert test data
data(diamonds, package="ggplot2")
mongo_stream_out(diamonds, m)

# Check records
mongo_collection_count(m)
nrow(diamonds)

# Perform a query and retrieve data
out <- mongo_stream_in(m, query = '{"cut" : "Premium", "price" : { "$lt" : 1000 } }')

# Compare
nrow(out)
nrow(subset(diamonds, cut == "Premium" & price < 1000))
```

Combine with jsonlite
---------------------

```r
library(jsonlite)
library(mongolite)

# Stream from url into mongo
m <- mongo_connect(collection = "zips")
stream_in(url("http://media.mongodb.org/zips.json"), handler = function(df){
  mongo_stream_out(df, m, verbose = FALSE)
})

# Check count
mongo_collection_count(m)

# Import. Note the 'location' column is actually an array!
zips <- mongo_stream_in(m)
```

Nested data
-----------

Example bulk data from [openweathermap](http://openweathermap.org/current#bulk) which contains deeply nested data. 

```r
m <- mongo_connect(collection = "weather")
stream_in(gzcon(url("http://78.46.48.103/sample/daily_14.json.gz")), handler = function(df){
  mongo_stream_out(df, m, verbose = FALSE)  
}, pagesize = 50)

berlin <- mongo_stream_in(m, query = '{"city.name" : "Berlin"}')
```
