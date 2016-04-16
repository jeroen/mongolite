# mongolite

##### *Fast and Simple MongoDB Client for R*

[![Build Status](https://travis-ci.org/jeroenooms/mongolite.svg?branch=master)](https://travis-ci.org/jeroenooms/mongolite)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/github/jeroenooms/mongolite?branch=master&svg=true)](https://ci.appveyor.com/project/jeroenooms/mongolite)
[![Coverage Status](https://codecov.io/github/jeroenooms/mongolite/coverage.svg?branch=master)](https://codecov.io/github/jeroenooms/mongolite?branch=master)
[![CRAN_Status_Badge](http://www.r-pkg.org/badges/version/mongolite)](http://cran.r-project.org/package=mongolite)
[![CRAN RStudio mirror downloads](http://cranlogs.r-pkg.org/badges/mongolite)](http://cran.r-project.org/web/packages/mongolite/index.html)
[![Research software impact](http://depsy.org/api/package/cran/mongolite/badge.svg)](http://depsy.org/package/r/mongolite)
[![Github Stars](https://img.shields.io/github/stars/jeroenooms/mongolite.svg?style=social&label=Github)](https://github.com/jeroenooms/mongolite)

> High-level, high-performance MongoDB client based on libmongoc and
  jsonlite. Includes support for aggregation, indexing, map-reduce, streaming,
  SSL encryption and SASL authentication. The vignette gives a brief overview
  of the available methods in the package.

## Documentation

About the R package:

 - Vignette: [Getting started with MongoDB in R](https://cran.r-project.org/web/packages/mongolite/vignettes/intro.html)

## Hello World

There are three download interfaces (memory, disk and streaming). Always start by setting up a request handle:

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

# Cross-table
tbl <- m$mapreduce(
  map = "function(){emit({cut:this.cut, color:this.color}, 1)}",
  reduce = "function(id, counts){return Array.sum(counts)}"
)
# Same as:
data.frame(with(diamonds, table(cut, color)))

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

Installation from source on Linux requires `openssl` and `libsasl2`. On __Debian__ or __Ubuntu__ use [libssl-dev](https://packages.debian.org/testing/libssl-dev) and [libsasl2-dev](https://packages.debian.org/testing/libsasl2-dev):

```
sudo apt-get install -y libssl-dev libsasl2-dev
```

On __Fedora__, __CentOS or RHEL__ use [openssl-devel](https://apps.fedoraproject.org/packages/openssl-devel) and [cyrus-sasl-devel](https://apps.fedoraproject.org/packages/cyrus-sasl-devel):

```
sudo yum install openssl-devel cyrus-sasl-devel
````

On __OS-X__ libsasl2 is included with the system so only [openssl](https://github.com/Homebrew/homebrew-core/blob/master/Formula/openssl.rb) is needed.

```
brew install openssl
```
