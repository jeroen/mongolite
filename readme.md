# mongolite

##### *Fast and Simple MongoDB Client for R*

[![Build Status](https://travis-ci.org/jeroen/mongolite.svg?branch=master)](https://travis-ci.org/jeroen/mongolite)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/github/jeroen/mongolite?branch=master&svg=true)](https://ci.appveyor.com/project/jeroen/mongolite)
[![Coverage Status](https://codecov.io/github/jeroen/mongolite/coverage.svg?branch=master)](https://codecov.io/github/jeroen/mongolite?branch=master)
[![CRAN_Status_Badge](http://www.r-pkg.org/badges/version/mongolite)](http://cran.r-project.org/package=mongolite)
[![CRAN RStudio mirror downloads](http://cranlogs.r-pkg.org/badges/mongolite)](http://cran.r-project.org/web/packages/mongolite/index.html)
[![Research software impact](http://depsy.org/api/package/cran/mongolite/badge.svg)](http://depsy.org/package/r/mongolite)
[![Github Stars](https://img.shields.io/github/stars/jeroen/mongolite.svg?style=social&label=Github)](https://github.com/jeroen/mongolite)

> High-level, high-performance MongoDB client based on libmongoc and
  jsonlite. Includes support for aggregation, indexing, map-reduce, streaming,
  SSL encryption and SASL authentication. The vignette gives a brief overview
  of the available methods in the package.

## Documentation

About the R package:

 - Book: [Mongolite User Manual](https://jeroen.github.io/mongolite/)
 - Presentation: [UseR 2015 slides](http://jeroen.github.io/mongo-slides)

## Hello World


Example using a public test server

```r
# Connect to demo server
con <- mongo("mtcars", url = "mongodb://readwrite:test@mongo.opencpu.org:43942/jeroen_test")

# Wipe collection
if(con$count() > 0) 
  con$drop()
  
# Insert some data
con$insert(mtcars)
stopifnot(con$count() == nrow(mtcars))

# Query data
mydata <- con$find()
stopifnot(all.equal(mydata, mtcars))
con$drop()

# Automatically disconnect when connection is removed
rm(con)
gc()
```

Insert/retrieve data from your local mongodb server:

```r
# Init connection to local mongod
library(mongolite)
m <- mongo(collection = "diamonds")

# Insert test data
data(diamonds, package="ggplot2")
m$insert(diamonds)

# Check records
m$count()
nrow(diamonds)

# Perform a query and retrieve data
out <- m$find('{"cut" : "Premium", "price" : { "$lt" : 1000 } }')

# Compare
nrow(out)
nrow(subset(diamonds, cut == "Premium" & price < 1000))
```

More advanced features include map reduce:

```r
# Cross-table
tbl <- m$mapreduce(
  map = "function(){emit({cut:this.cut, color:this.color}, 1)}",
  reduce = "function(id, counts){return Array.sum(counts)}"
)
# Same as:
data.frame(with(diamonds, table(cut, color)))
```

Importing and exporting json or bson data:

```r
# Stream jsonlines into a connection
tmp <- tempfile()
m$export(file(tmp))

# Stream it back in R
library(jsonlite)
mydata <- stream_in(file(tmp))

# Or into mongo
m2 <- mongo("diamonds2")
m2$count()
m2$import(file(tmp))
m2$count()

# Remove the collection
m$drop()
m2$drop()
```

## Installation

Binary packages for __OS-X__ or __Windows__ can be installed directly from CRAN:

```r
install.packages("mongolite")
```

Installation from source on Linux requires `openssl` and `Cyrus SASL` (**not** `GNU sasl`). On __Debian__ or __Ubuntu__ use [libssl-dev](https://packages.debian.org/testing/libssl-dev) and [libsasl2-dev](https://packages.debian.org/testing/libsasl2-dev):

```
sudo apt-get install -y libssl-dev libsasl2-dev
```

On __Fedora__, __CentOS or RHEL__ use [openssl-devel](https://apps.fedoraproject.org/packages/openssl-devel) and [cyrus-sasl-devel](https://apps.fedoraproject.org/packages/cyrus-sasl-devel):

```
sudo yum install openssl-devel cyrus-sasl-devel
````

On __OS-X__ sasl is included with the system so only [openssl](https://github.com/Homebrew/homebrew-core/blob/master/Formula/openssl.rb) is needed.

```
brew install openssl
```
