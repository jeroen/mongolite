--- 
title: "Mongolite User Manual"
date: "Version 2017-03-01"
site: bookdown::bookdown_site
output: bookdown::gitbook
output_dir: "docs"
link-citations: yes
github-repo: jeroenooms/mongobook
description: "A brief introduction to MongoDB and mongolite for R users"
---




# Getting Started

This book provides a high level introduction to using MongoDB with the **mongolite** client in R. 

## Requirements (Linux / Mac)

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

On __Windows__ all dependencies are statically linked with mongolite; no separate installation is required.
Elsewhere, mongolite will automatically find the system libraries when installed in default locations.


## Install mongolite in R

Binary packages of **mongolite** for __OS-X__ or __Windows__ can be installed directly from CRAN:


```r
install.packages("mongolite")
```

Alternatively you can install the development version, which contains the latest features and bugs. This required compiling from source:


```r
# installs development version of 'mongolite'
devtools::install_github("jeroenooms/mongolite")
```

Note that Windows users need to install the [Rtools](https://cran.r-project.org/bin/windows/Rtools/) toolchain in order to compile from source. This is not needed for the CRAN version above.

## Run local mongod

Please refer to the official MongoDB [install manual](https://docs.mongodb.com/manual/installation/) for
instructions how to setup a local MongoDB server. To get started on MacOS, simply use:

```
brew install mongodb
```

The Homebrew package does not install any system services. To run the daemon in a console:

```
mongod
```

Use this for running examples as the mongolite package defaults to ` url = "mongodb://localhost"`.

## Testing with SSL

To run a local `mongod` with SSL support you need a SSL key and certificate. See the
[Mongo Configure SSL](https://docs.mongodb.com/manual/tutorial/configure-ssl/) manual page.
A generate a self signed cert for testing purposes:

```
cd /etc/ssl/
openssl req -newkey rsa:2048 -new -x509 -days 365 -nodes -out mongodb-cert.crt -keyout mongodb-cert.key
cat mongodb-cert.key mongodb-cert.crt > mongodb.pem
```

And then start the daemon with:

```
mongod --sslMode requireSSL --sslPEMKeyFile /etc/ssl/mongodb.pem
```

Because this certificate has not been signed with a CA, you need to set `weak_cert_validation` in the client
to connect:

```r
m <- mongo(url = "mongodb://localhost?ssl=true", options = ssl_options(weak_cert_validation = T))
```

Obviously in production you need to get your cert signed by a CA instead.

<!--chapter:end:index.Rmd-->

# Connecting to MongoDB

## Mongo URI Format

The `mongo()` function initiates a connection object to a MongoDB server. For example:


```r
library(mongolite)
m <- mongo("mtcars", url = "mongodb://readwrite:test@ds043942.mongolab.com:43942/jeroen_test")
```

To get an overview of available methods, simply print the object to the terminal. 


```r
print(m)
```
```{.outputcode}
#> <Mongo collection> 'mtcars' 
#>  $aggregate(pipeline = "{}", options = "{\"allowDiskUse\":true}", handler = NULL, pagesize = 1000) 
#>  $count(query = "{}") 
#>  $distinct(key, query = "{}") 
#>  $drop() 
#>  $export(con = stdout(), bson = FALSE) 
#>  $find(query = "{}", fields = "{\"_id\":0}", sort = "{}", skip = 0, limit = 0, handler = NULL, pagesize = 1000) 
#>  $import(con, bson = FALSE) 
#>  $index(add = NULL, remove = NULL) 
#>  $info() 
#>  $insert(data, pagesize = 1000, ...) 
#>  $iterate(query = "{}", fields = "{\"_id\":0}", sort = "{}", skip = 0, limit = 0) 
#>  $mapreduce(map, reduce, query = "{}", sort = "{}", limit = 0, out = NULL, scope = NULL) 
#>  $remove(query, just_one = FALSE) 
#>  $rename(name, db = NULL) 
#>  $update(query, update = "{\"$set\":{}}", upsert = FALSE, multiple = FALSE)
```

The R manual page for the `mongo()` function gives some brief descriptions as well.

```r
?mongo
```

The manual page tells us that `mongo()` supports the following arguments:

 - `collection`: name of the collection to connect to. Defaults to `"test"`.
 - `db`: name of the database to connect to. Defaults to `"test"`.
 - `url`: address of the MongoDB server in standard [URI Format](https://docs.mongodb.com/manual/reference/connection-string/).
 - `verbose`: if `TRUE`, emits some extra output
 - `options`: additinoal connection options such as SSL keys/certs.
 
The `url` parameter contains a special URI format which defines the server address
and additional connection options. 

```
mongodb://[username:password@]host1[:port1][,host2[:port2],...[/[database][?options]]
```

The [Mongo Connection String Manual](https://docs.mongodb.com/manual/reference/connection-string/)
gives an overview of the connection string syntax and options. 
Below the most important options for using mongolite.



## Authentication

MongoDB supports several authentication modes.

### LDAP


```r
USER = "drivers-team"
PASS = "mongor0x$xgen"
HOST = "ldaptest.10gen.cc"

# Using plain-text
URI = sprintf("mongodb://%s:%s@%s/ldap?authMechanism=PLAIN", USER, PASS, HOST)
m <- mongo(url = URI)
m$find()
```

However it is recommended to use SSL instead of plain text when authenticating with a username/password: 


```r
# Or over SSL
m <- mongo(url = paste0(URI, "&ssl=true"), options = 
             ssl_options(ca = "auth/ca.crt", allow_invalid_hostname = TRUE))
m$find()
```

### X509

Let's check if our server supports SSL:


```r
certs <- openssl::download_ssl_cert('ldaptest.10gen.cc', 27017)
print(certs)
str(as.list(certs[[1]]))
```

To use X509 authentication the Mongo URI needs `ssl=true&authMechanism=MONGODB-X509`:


```r
# Using X509 SSL auth
HOST <- "ldaptest.10gen.cc"
USER <- "CN=client,OU=kerneluser,O=10Gen,L=New York City,ST=New York,C=US"
URI <- sprintf("mongodb://%s@%s/x509?ssl=true&authMechanism=MONGODB-X509", USER, HOST)
OPTS <- ssl_options(cert = "auth/client.pem", key = "auth/key.pem", ca = "auth/ca.crt", allow_invalid_hostname = TRUE)
m <- mongo(url = URI, options = OPTS)
m$find()
```

### Kerberos

*__Note__: Windows uses `SSPI` for Kerberos authentication. This section does not apply*.  

Kerberos authentication on Linux requires installation of a Kerberos client. 
On OS-X Kerberos is already installed by default. 
On Ubuntu/Debian we need:

```
sudo apt-get install krb5-user libsasl2-modules-gssapi-mit
```

Next, create or edit `/etc/krbs5.conf` and add our server under `[realms]` for example:

```
[realms]
  LDAPTEST.10GEN.CC = {
    kdc = ldaptest.10gen.cc
    admin_server = ldaptest.10gen.cc
  }
```

In a terminal run the following (only have to do this once)

```
kinit drivers@LDAPTEST.10GEN.CC
klist
```

We should now be able to connect in R:


```r
HOST <- "ldaptest.10gen.cc"
USER <- "drivers%40LDAPTEST.10GEN.CC"
URI <- sprintf("mongodb://%s@%s/kerberos?authMechanism=GSSAPI", USER, HOST)
m <- mongo(url = URI)
m$find()
```


## SSH Tunnel

To connect to MongoDB via an SSH tunnel, you need to setup the tunnel separately with an SSH client. For example the mongolite manual contains this example:

```r
con <- mongo("mtcars", url = "mongodb://readwrite:test@ds043942.mongolab.com:43942/jeroen_test")
```

Assume we want to tunnel through `dev.opencpu.org` which runs an SSH server on the standard port 22 with username `jeroen`. To initiate a tunnel from `localhost:9999` to `ds043942.mongolab.com:43942` via the ssh server `dev.opencpu.org:22`, open a terminal and run:

```
ssh -L 9999:ds043942.mongolab.com:43942 jeroen@dev.opencpu.org -vN -p22
```

Some relevant `ssh` flags:

 - `-v` (optional) show verbose status output
 - `-f` run the tunnel server in the background. Use `pkill ssh` to kill.
 - `-p22` connect to ssh server on port 22 (default)
 - `-i/some/path/id_rsa` authenticate with ssh using a private key

Check `man ssh` for more ssh options It is also possible to run this command directly from R:


```r
system("ssh -L 9999:ds043942.mongolab.com:43942 jeroen@dev.opencpu.org -fN -p22")
```

Once tunnel has been established, we can connect to our our ssh client which will tunnel traffic to our MongoDB server. In our example we run the ssh client on our localhost port 9999:


```r
con <- mongo("mtcars", url = "mongodb://readwrite:test@localhost:9999/jeroen_test")
con$insert(mtcars)
```
```{.outputcode}
#> List of 5
#>  $ nInserted  : num 32
#>  $ nMatched   : num 0
#>  $ nRemoved   : num 0
#>  $ nUpserted  : num 0
#>  $ writeErrors: list()
```



If you want to setup a tunnel client on Windows and you do not have the `ssh` program, you can an SSH client like putty to setup the tunnel. See [this example](http://realprogrammers.com/how_to/set_up_an_ssh_tunnel_with_putty.html).


## SSL options

For security reasons, SSL options can not be configured in the URI but have to be set 
manually via the `options` parameter. The `ssl_options` function shows the default values:


```r
ssl_options()
```
```{.outputcode}
#> List of 6
#>  $ pem_file              : NULL
#>  $ ca_file               : NULL
#>  $ ca_dir                : NULL
#>  $ crl_file              : NULL
#>  $ allow_invalid_hostname: logi FALSE
#>  $ weak_cert_validation  : logi FALSE
```

You can use this function to specify connection SSL options:

```r
m <- mongo(url = "mongodb://localhost?ssl=true", 
  options = ssl_options(cert = "~/client.crt", key = "~/id_rsa.pem"))
```

The MongoDB [SSL client manual](https://docs.mongodb.com/manual/tutorial/configure-ssl-clients/)
has more detailed descriptions on the various options.

## Replica Options

The URI accepts a few special keys when connecting to a replicaset. The
[connection-string manual](https://docs.mongodb.com/manual/reference/connection-string/)
is the canonical source for all parameters. Most users should stick with the
defaults here, only specify these if you know what you are doing.


### Read Preference

The **Read Preference** paremeter specifies if the client should connect to the 
primary node (default) or a secondary node in the replica set. 

```r
m <- mongo(url = "mongodb://host.example.com/?readPreference=secondary")
```

### Write Concern

The **Write Concern** parameter is used to specify the level of acknowledgement that 
the write operation has propagated to a number of server nodes. The url string parameter
is the letter `w`.

```r
m <- mongo(url = "mongodb://host.example.com/?w=2")
```

Note that specifying this parameter to 2 on a server that is not a replicaset will result in an error when trying to write:


```r
m <- mongo(url = "mongodb://localhost/?w=2")
m$insert('{"foo" : "bar"}')
```
```{.outputcode}
#> Error: cannot use 'w' > 1 when a host is not replicated
```

### Read Concern

Finally, **Read Concern** allows clients to choose a level of isolation for their reads from replica sets. The default value `local` returns the instance’s most recent data, but provides no guarantee that the data has been written to a majority of the replica set members (i.e. may be rolled back).

On the other hand, if we specify `majority` the server will only return data that has been propagated to the majority of nodes.


```r
m <- mongo(url = "mongodb://localhost/?readConcernLevel=majority")
m$insert('{"foo": 123}')
```
```{.outputcode}
#> List of 6
#>  $ nInserted  : int 1
#>  $ nMatched   : int 0
#>  $ nModified  : int 0
#>  $ nRemoved   : int 0
#>  $ nUpserted  : int 0
#>  $ writeErrors: list()
```

```r
m$insert('{"foo": 456}')
```
```{.outputcode}
#> List of 6
#>  $ nInserted  : int 1
#>  $ nMatched   : int 0
#>  $ nModified  : int 0
#>  $ nRemoved   : int 0
#>  $ nUpserted  : int 0
#>  $ writeErrors: list()
```

In the case of our local single-node server this is never the case. Therefore we see that the server does not return any data that meets the majority level.


```r
m$count()
```
```{.outputcode}
#> Error: Majority read concern requested, but server was not started with --enableMajorityReadConcern.
```

```r
m$find()
```
```{.outputcode}
#> data frame with 0 columns and 0 rows
```

The data is definitely there though, it just doesn't meet the `majority` criterium. If we create a new connection with level `local` we do get to see our data:


```r
m2 <- mongo(url = "mongodb://localhost/?readConcernLevel=local")
m2$count()
```
```{.outputcode}
#> [1] 2
```

```r
m2$find()
```
```{.outputcode}
#>   foo
#> 1 123
#> 2 456
```

## Global options

Finally the `mongo_options` method allows for setting global client options that span across connections. Currently two options are supported:

 - `log_level` set the mongo [log level](http://mongoc.org/libmongoc/current/logging.html), e.g. for printing debugging information.
 - `bigint_as_char` set to `TRUE` to parse int64 numbers as strings rather than doubles (R does not support large integers natively)
 
The default values are:


```r
mongo_options()
```
```{.outputcode}
#> $log_level
#> [1] "INFO"
#> 
#> $bigint_as_char
#> [1] FALSE
```
 
See the manual page for `?mongo_options` for more details.

<!--chapter:end:01-connecting.Rmd-->

# Query Data

This chapter will cover basic techniques for reading data from MongoDB. To exemplify this chapter
we start by creating a new collection **diamonds** and insert an example dataset from the __ggplot2__ package.


```r
# create collection with example data
dmd <- mongo("diamonds")
dmd$insert(ggplot2::diamonds)
```
```{.outputcode}
#> List of 5
#>  $ nInserted  : num 53940
#>  $ nMatched   : num 0
#>  $ nRemoved   : num 0
#>  $ nUpserted  : num 0
#>  $ writeErrors: list()
```

The next chapter explains inserting data in more detail. For now let's verify all our data was inserted:


```r
dmd$count() == nrow(ggplot2::diamonds)
```
```{.outputcode}
#> [1] TRUE
```

Seems good!

## Query syntax

MongoDB uses [JSON based syntax](https://docs.mongodb.com/manual/tutorial/query-documents/) to query documents. The empty query `{}` means: select all data. The same query parameter is used for multiple operations such as `find()`, `iterate()`, `count()`, `remove()` and `update()`. We need to specify the JSON query as a string in R.



```r
# Get all records
dmd$count('{}')
```
```{.outputcode}
#> [1] 53940
```

```r
# Read all the data back into R
alldata <- dmd$find('{}')
print(alldata)
```
```{.outputcode}
#>       carat       cut color clarity depth table price     x     y     z
#> 1      0.23     Ideal     E     SI2  61.5  55.0   326  3.95  3.98  2.43
#> 2      0.21   Premium     E     SI1  59.8  61.0   326  3.89  3.84  2.31
#> 3      0.23      Good     E     VS1  56.9  65.0   327  4.05  4.07  2.31
#> 4      0.29   Premium     I     VS2  62.4  58.0   334  4.20  4.23  2.63
#> 5      0.31      Good     J     SI2  63.3  58.0   335  4.34  4.35  2.75
#> 6      0.24 Very Good     J    VVS2  62.8  57.0   336  3.94  3.96  2.48
#> 7      0.24 Very Good     I    VVS1  62.3  57.0   336  3.95  3.98  2.47
#> 8      0.26 Very Good     H     SI1  61.9  55.0   337  4.07  4.11  2.53
#> 9      0.22      Fair     E     VS2  65.1  61.0   337  3.87  3.78  2.49
#> 10     0.23 Very Good     H     VS1  59.4  61.0   338  4.00  4.05  2.39
#>  [ reached getOption("max.print") -- omitted 53930 rows ]
```

To query all rows where `cut == "Premium" AND price < 1000` you would run:


```r
premium_diamonds <- dmd$find('{"cut" : "Premium", "price" : { "$lt" : 1000 } }')
print(premium_diamonds)
```
```{.outputcode}
#>      carat     cut color clarity depth table price    x    y    z
#> 1     0.21 Premium     E     SI1  59.8  61.0   326 3.89 3.84 2.31
#> 2     0.29 Premium     I     VS2  62.4  58.0   334 4.20 4.23 2.63
#> 3     0.22 Premium     F     SI1  60.4  61.0   342 3.88 3.84 2.33
#> 4     0.20 Premium     E     SI2  60.2  62.0   345 3.79 3.75 2.27
#> 5     0.32 Premium     E      I1  60.9  58.0   345 4.38 4.42 2.68
#> 6     0.24 Premium     I     VS1  62.5  57.0   355 3.97 3.94 2.47
#> 7     0.29 Premium     F     SI1  62.4  58.0   403 4.24 4.26 2.65
#> 8     0.22 Premium     E     VS2  61.6  58.0   404 3.93 3.89 2.41
#> 9     0.22 Premium     D     VS2  59.3  62.0   404 3.91 3.88 2.31
#> 10    0.30 Premium     J     SI2  59.3  61.0   405 4.43 4.38 2.61
#>  [ reached getOption("max.print") -- omitted 3190 rows ]
```

We can confirm that we get the same subset in R:


```r
nrow(premium_diamonds)
```
```{.outputcode}
#> [1] 3200
```

```r
nrow(subset(ggplot2::diamonds, cut == "Premium" & price < 1000))
```
```{.outputcode}
#> [1] 3200
```

To learn more about mongo data queries, study the 
[Mongo Query Documents](https://docs.mongodb.com/manual/tutorial/query-documents/) manual.


## Filter fields

The `fields` parameter filters specific columns from the output. Let's continue our example above:


```r
test <- dmd$find(
  query = '{"cut" : "Premium", "price" : { "$lt" : 1000 } }', 
  fields = '{"cut" : true, "clarity" : true}',
  limit = 5
)
print(test)
```
```{.outputcode}
#>                        _id     cut clarity
#> 1 58b6f04fba7cb2346a6ec4b3 Premium     SI1
#> 2 58b6f04fba7cb2346a6ec4b5 Premium     VS2
#> 3 58b6f04fba7cb2346a6ec4be Premium     SI1
#> 4 58b6f04fba7cb2346a6ec4c0 Premium     SI2
#> 5 58b6f04fba7cb2346a6ec4c1 Premium      I1
```

By default mongo always includes the `id` field. To prevent this we need to explicitly disable it:


```r
test <- dmd$find(
  query = '{"cut" : "Premium", "price" : { "$lt" : 1000 } }', 
  fields = '{"cut" : true, "clarity" : true, "_id": false}',
  limit = 5
)
print(test)
```
```{.outputcode}
#>       cut clarity
#> 1 Premium     SI1
#> 2 Premium     VS2
#> 3 Premium     SI1
#> 4 Premium     SI2
#> 5 Premium      I1
```

The default value for the `field` argument is `'{"_id" : 0}'` i.e. all columns, except for `_id`.

## Sort and limit

The `sort` paramter allows us to order the output, and `limit` restricts the number records that will be returned. For example to return the 7 most expensive __premium__ diamonds in the data we sort by price in descending order:


```r
dmd$find('{"cut" : "Premium"}', sort = '{"price": -1}', limit = 7)
```
```{.outputcode}
#>   carat     cut color clarity depth table price    x    y    z
#> 1  2.29 Premium     I     VS2  60.8    60 18823 8.50 8.47 5.16
#> 2  2.29 Premium     I     SI1  61.8    59 18797 8.52 8.45 5.24
#> 3  2.04 Premium     H     SI1  58.1    60 18795 8.37 8.28 4.84
#> 4  2.00 Premium     I     VS1  60.8    59 18795 8.13 8.02 4.91
#> 5  1.71 Premium     F     VS2  62.3    59 18791 7.57 7.53 4.70
#> 6  2.05 Premium     F     SI2  60.2    59 18784 8.28 8.33 5.00
#> 7  2.55 Premium     I     VS1  61.8    62 18766 8.70 8.65 5.36
```

Note that usually you should only sort by fields that have an index set on them. Sorting by unindexed fields can be very slow and the server might reach the memory limits on the server. 


## Indexing

By default a collection only has an index for `_id`, which means that selecting or sorting by any other field is relatively slow. 


```r
system.time(dmd$find(sort = '{"price" : 1}', limit = 1))
```
```{.outputcode}
#>    user  system elapsed 
#>   0.002   0.000   0.124
```

By adding an index, the field gets presorted and selecting or sorting it is almost immediate:


```r
dmd$index(add = '{"price" : 1}')
```
```{.outputcode}
#>   v key._id key.price    name            ns
#> 1 2       1        NA    _id_ test.diamonds
#> 2 2      NA         1 price_1 test.diamonds
```

```r
system.time(dmd$find(sort = '{"price" : 1}', limit = 1))
```
```{.outputcode}
#>    user  system elapsed 
#>   0.001   0.000   0.003
```

In order to speed up queries involving multiple fields, you need to add a cross-index which intersects both fields:


```r
dmd$index(add = '{"depth" : 1}')
```
```{.outputcode}
#>   v key._id key.price key.depth    name            ns
#> 1 2       1        NA        NA    _id_ test.diamonds
#> 2 2      NA         1        NA price_1 test.diamonds
#> 3 2      NA        NA         1 depth_1 test.diamonds
```

```r
dmd$index(add = '{"depth" : 1, "price" : 1}')
```
```{.outputcode}
#>   v key._id key.price key.depth            name            ns
#> 1 2       1        NA        NA            _id_ test.diamonds
#> 2 2      NA         1        NA         price_1 test.diamonds
#> 3 2      NA        NA         1         depth_1 test.diamonds
#> 4 2      NA         1         1 depth_1_price_1 test.diamonds
```

To remove indices from the collection, use the `name` as listed above:


```r
dmd$index(remove = 'depth_1_price_1')
```
```{.outputcode}
#>   v key._id key.price key.depth    name            ns
#> 1 2       1        NA        NA    _id_ test.diamonds
#> 2 2      NA         1        NA price_1 test.diamonds
#> 3 2      NA        NA         1 depth_1 test.diamonds
```

```r
dmd$index(remove = 'depth_1')
```
```{.outputcode}
#>   v key._id key.price    name            ns
#> 1 2       1        NA    _id_ test.diamonds
#> 2 2      NA         1 price_1 test.diamonds
```


## Iterating

The `find()` method automatically simplifies the collection into a data frame, but sometimes you need more fine-grained control. The `iterate()` method allows you to perform a query, but read the records one-by-one without simplification. 

The iterator has methods `one()`, `batch(n)` which allow you to step through a single or `n` records at the time. When the iterator is exhausted it will return `NULL`. Lets run the same query as above using the iterator interface:


```r
# perform query and return the iterator
it <- dmd$iterate('{"cut" : "Premium"}', sort = '{"price": -1}', limit = 7)

# read records from  the iterator
while(!is.null(x <- it$one())){
  cat(sprintf("Found %.2f carat diamond for $%d\n", x$carat, x$price))
}
```
```{.outputcode}
#> Found 2.29 carat diamond for $18823
#> Found 2.29 carat diamond for $18797
#> Found 2.00 carat diamond for $18795
#> Found 2.04 carat diamond for $18795
#> Found 1.71 carat diamond for $18791
#> Found 2.05 carat diamond for $18784
#> Found 2.55 carat diamond for $18766
```

The iterator does not perform any simplification, so each `x` is simply a named list containing the parsed JSON record.

## Select by date

In order to query by timestamp we must make sure dates are in proper **UTC datetime** format. When inserting data via R this means
the column must be in `POSIXct` type.


```r
# Get some example data
mydata <- jsonlite::fromJSON("https://api.github.com/repos/jeroenooms/mongolite/issues")
mydata$created_at <- strptime(mydata$created_at, "%Y-%m-%dT%H:%M:%SZ", 'UTC')
mydata$closed_at <- strptime(mydata$closed_at, "%Y-%m-%dT%H:%M:%SZ", 'UTC')

# Insert into mongo
issues <- mongo("issues")
issues$insert(mydata)
```
```{.outputcode}
#> List of 5
#>  $ nInserted  : num 9
#>  $ nMatched   : num 0
#>  $ nRemoved   : num 0
#>  $ nUpserted  : num 0
#>  $ writeErrors: list()
```

Selecting by date is done via the `"$date"` operator. For example to select dates which were created after January 1st, 2017:


```r
issues$find(
  query = '{"created_at": { "$gte" : { "$date" : "2017-01-01T00:00:00Z" }}}',
  fields = '{"created_at" : true, "user.login" : true, "title":true, "_id": false}'
)
```
```{.outputcode}
#>            title      login created_at
#> 1 Simplify dates jeroenooms 1488214306
```

Note that confusingly, what R calls a *Date* is not a timestamp but rather a string which only contains the date (Y-M-D) part of a timestamp. This type cannot be queried in MongoDB.

See the MongoDB Extended JSON [manual](https://docs.mongodb.com/manual/reference/mongodb-extended-json/#date) for details.


## Select by ID

Each record inserted into MongoDB is automatically assigned an `"_id"` value. 


```r
issues$find(fields= '{"created_at":true, "_id":true}', limit = 10)
```
```{.outputcode}
#>                        _id created_at
#> 1 58b6f053ba7cb2346a6f9767 1488214306
#> 2 58b6f053ba7cb2346a6f9768 1482331814
#> 3 58b6f053ba7cb2346a6f9769 1473432217
#> 4 58b6f053ba7cb2346a6f976a 1471991562
#> 5 58b6f053ba7cb2346a6f976b 1466094279
#> 6 58b6f053ba7cb2346a6f976c 1462450923
#> 7 58b6f053ba7cb2346a6f976d 1439313015
#> 8 58b6f053ba7cb2346a6f976e 1430538700
#> 9 58b6f053ba7cb2346a6f976f 1430101791
```

Use the `{"$oid"}` operator (similar to `ObjectId()` in mongo) to select a record by it's ID, for example:

```r
issues$find(query = '{"_id" : {"$oid":"58a83a6aba7cb2070210433e"}}')
```

See the MongoDB Extended JSON [manual](https://docs.mongodb.com/manual/reference/mongodb-extended-json/#oid) for syntax details.


<!--chapter:end:02-queries.Rmd-->

# Manipulate Data

## Insert

The easiest way to insert data is from an R data frame. The data frame columns will automatically be mapped to keys
in the JSON records:


```r
test <- mongo()
test$drop()
test$insert(iris)
```
```{.outputcode}
#> List of 5
#>  $ nInserted  : num 150
#>  $ nMatched   : num 0
#>  $ nRemoved   : num 0
#>  $ nUpserted  : num 0
#>  $ writeErrors: list()
```

This is basically the inverse from `mongo$find()` which converts the collection back into a Data Frame:


```r
test$find(limit = 5)
```
```{.outputcode}
#>   Sepal_Length Sepal_Width Petal_Length Petal_Width Species
#> 1          5.1         3.5          1.4         0.2  setosa
#> 2          4.9         3.0          1.4         0.2  setosa
#> 3          4.7         3.2          1.3         0.2  setosa
#> 4          4.6         3.1          1.5         0.2  setosa
#> 5          5.0         3.6          1.4         0.2  setosa
```

Records can also be inserted directly as JSON strings. This requires a character vector where each element is a valid JSON string.


```r
subjects <- mongo("subjects")
str <- c('{"name" : "jerry"}' , '{"name": "anna", "age" : 23}', '{"name": "joe"}')
subjects$insert(str)
```
```{.outputcode}
#> List of 6
#>  $ nInserted  : int 3
#>  $ nMatched   : int 0
#>  $ nModified  : int 0
#>  $ nRemoved   : int 0
#>  $ nUpserted  : int 0
#>  $ writeErrors: list()
```

```r
subjects$find(query = '{}', fields = '{}')
```
```{.outputcode}
#>                        _id  name age
#> 1 58b6f053ba7cb2346a6f9808 jerry  NA
#> 2 58b6f053ba7cb2346a6f9809  anna  23
#> 3 58b6f053ba7cb2346a6f980a   joe  NA
```

Obviously you can generate such JSON records dynamically from data via e.g. `jsonlite::toJSON`.

## Remove

The same syntax that we use in `find()` to select records for reading, can also be used to select records for deleting:


```r
test$count()
```
```{.outputcode}
#> [1] 150
```

```r
test$remove('{"Species" : "setosa"}')
test$count()
```
```{.outputcode}
#> [1] 100
```

Use the `just_one` option to delete a single record:


```r
test$remove('{"Sepal_Length" : {"$lte" : 5}}', just_one = TRUE)
test$count()
```
```{.outputcode}
#> [1] 99
```

To delete all records in the collection (but not the collection itself):


```r
test$remove('{}')
test$count()
```
```{.outputcode}
#> [1] 0
```

The `drop()` operator will delete an entire collection. This includes all data, as well as metadata such as collection indices.


```r
test$drop()
```


## Update / Upsert

To modify existing records, use the `update()` operator:


```r
subjects$find()
```
```{.outputcode}
#>    name age
#> 1 jerry  NA
#> 2  anna  23
#> 3   joe  NA
```

```r
subjects$update('{"name":"jerry"}', '{"$set":{"age": 31}}')
subjects$find()
```
```{.outputcode}
#>    name age
#> 1 jerry  31
#> 2  anna  23
#> 3   joe  NA
```

By default, the update() method updates a single document. To update multiple documents, use the multi option in the update() method. 


```r
subjects$update('{}', '{"$set":{"has_age": false}}', multiple = TRUE)
subjects$update('{"age" : {"$gte" : 0}}', '{"$set":{"has_age": true}}', multiple = TRUE)
subjects$find()
```
```{.outputcode}
#>    name age has_age
#> 1 jerry  31    TRUE
#> 2  anna  23    TRUE
#> 3   joe  NA   FALSE
```

If no document matches the update condition, the default behavior of the update method is to do nothing. By specifying the upsert option to true, the update operation either updates matching document(s) or inserts a new document if no matching document exists. 


```r
subjects$update('{"name":"erik"}', '{"$set":{"age": 29}}', upsert = TRUE)
subjects$find()
```
```{.outputcode}
#>    name age has_age
#> 1 jerry  31    TRUE
#> 2  anna  23    TRUE
#> 3   joe  NA   FALSE
#> 4  erik  29      NA
```


<!--chapter:end:03-insert.Rmd-->

# Import / Export

The `import()` and `export()` methods are used to read / write collection dumps via a connection, such as a file, socket or URL. 

## JSON

The default format for is newline delimited JSON lines, i.e. one line for each record (aka [NDJSON](http://ndjson.org/))


```r
subjects$export(stdout())
```
```{.outputcode}
#> { "_id" : { "$oid" : "58b6f053ba7cb2346a6f9808" }, "name" : "jerry", "age" : 31, "has_age" : true }
#> { "_id" : { "$oid" : "58b6f053ba7cb2346a6f9809" }, "name" : "anna", "age" : 23, "has_age" : true }
#> { "_id" : { "$oid" : "58b6f053ba7cb2346a6f980a" }, "name" : "joe", "has_age" : false }
#> { "_id" : { "$oid" : "58b6f054fe4250294bf7ca2d" }, "name" : "erik", "age" : 29 }
```

Usually we will export to a file:


```r
dmd$export(file("dump.json"))
```

Let's test this by removing the entire collection, and then importing it back from the file:


```r
dmd$drop()
dmd$count()
```
```{.outputcode}
#> [1] 0
```

```r
dmd$import(file("dump.json"))
dmd$count()
```
```{.outputcode}
#> [1] 53940
```

## Via jsonlite

The `jsonlite` package also allows for importing/exporting the NDJSON format directly in R via the `stream_in` and `stream_out` methods:


```r
mydata <- jsonlite::stream_in(file("dump.json"), verbose = FALSE)
print(mydata)
```
```{.outputcode}
#>                           $oid carat       cut color clarity depth table price     x     y     z
#> 1     58b6f04fba7cb2346a6ec4b2  0.23     Ideal     E     SI2  61.5  55.0   326  3.95  3.98  2.43
#> 2     58b6f04fba7cb2346a6ec4b3  0.21   Premium     E     SI1  59.8  61.0   326  3.89  3.84  2.31
#> 3     58b6f04fba7cb2346a6ec4b4  0.23      Good     E     VS1  56.9  65.0   327  4.05  4.07  2.31
#> 4     58b6f04fba7cb2346a6ec4b5  0.29   Premium     I     VS2  62.4  58.0   334  4.20  4.23  2.63
#> 5     58b6f04fba7cb2346a6ec4b6  0.31      Good     J     SI2  63.3  58.0   335  4.34  4.35  2.75
#> 6     58b6f04fba7cb2346a6ec4b7  0.24 Very Good     J    VVS2  62.8  57.0   336  3.94  3.96  2.48
#> 7     58b6f04fba7cb2346a6ec4b8  0.24 Very Good     I    VVS1  62.3  57.0   336  3.95  3.98  2.47
#> 8     58b6f04fba7cb2346a6ec4b9  0.26 Very Good     H     SI1  61.9  55.0   337  4.07  4.11  2.53
#> 9     58b6f04fba7cb2346a6ec4ba  0.22      Fair     E     VS2  65.1  61.0   337  3.87  3.78  2.49
#>  [ reached getOption("max.print") -- omitted 53931 rows ]
```

This is a convenient way to exchange data in a way with R users that might not have MongoDB. Similarly `jsonlite` allows for exporting data in a way that is easy to import in Mongo:


```r
jsonlite::stream_out(mtcars, file("mtcars.json"), verbose = FALSE)
mt <- mongo("mtcars")
mt$import(file("mtcars.json"))
mt$find()
```
```{.outputcode}
#>                      mpg cyl  disp  hp drat    wt  qsec vs am gear carb
#> Mazda RX4           21.0   6 160.0 110 3.90 2.620 16.46  0  1    4    4
#> Mazda RX4 Wag       21.0   6 160.0 110 3.90 2.875 17.02  0  1    4    4
#> Datsun 710          22.8   4 108.0  93 3.85 2.320 18.61  1  1    4    1
#> Hornet 4 Drive      21.4   6 258.0 110 3.08 3.215 19.44  1  0    3    1
#> Hornet Sportabout   18.7   8 360.0 175 3.15 3.440 17.02  0  0    3    2
#> Valiant             18.1   6 225.0 105 2.76 3.460 20.22  1  0    3    1
#> Duster 360          14.3   8 360.0 245 3.21 3.570 15.84  0  0    3    4
#> Merc 240D           24.4   4 146.7  62 3.69 3.190 20.00  1  0    4    2
#> Merc 230            22.8   4 140.8  95 3.92 3.150 22.90  1  0    4    2
#>  [ reached getOption("max.print") -- omitted 23 rows ]
```

## Streaming

Both `mongolite` and `jsonlite` also allow for importing NDJSON data from a HTTP stream:


```r
flt <- mongo("flights")
flt$import(gzcon(curl::curl("https://jeroenooms.github.io/data/nycflights13.json.gz")))
flt$count()
```
```{.outputcode}
#> [1] 336776
```

The same operation in `jsonlite` would be:


```r
flights <- jsonlite::stream_in(
  gzcon(curl::curl("https://jeroenooms.github.io/data/nycflights13.json.gz")), verbose = FALSE)
nrow(flights)
```
```{.outputcode}
#> [1] 336776
```

## BSON

MongoDB internally stores data in [BSON](http://bsonspec.org/spec.html) format, which is a binary version of JSON. Use the `bson` parameter to dump a collection directly in BSON format:


```r
flt$export(file("flights.bson"), bson = TRUE)
```

Same to read it back:


```r
flt$drop()
flt$import(file("flights.bson"), bson = TRUE)
```
```{.outputcode}
#> [1] 336776
```

```r
flt$find(limit = 5)
```
```{.outputcode}
#>   year month day dep_time dep_delay arr_time arr_delay carrier tailnum flight origin dest air_time distance hour minute
#> 1 2013     1   1      517         2      830        11      UA  N14228   1545    EWR  IAH      227     1400    5     17
#> 2 2013     1   1      533         4      850        20      UA  N24211   1714    LGA  IAH      227     1416    5     33
#> 3 2013     1   1      542         2      923        33      AA  N619AA   1141    JFK  MIA      160     1089    5     42
#> 4 2013     1   1      544        -1     1004       -18      B6  N804JB    725    JFK  BQN      183     1576    5     44
#> 5 2013     1   1      554        -6      812       -25      DL  N668DN    461    LGA  ATL      116      762    5     54
```

Using BSON to import/export is faster than JSON, but the resulting file can only be read by MongoDB.  


<!--chapter:end:04-export.Rmd-->

# Calculation

MongoDB has two methods for performing in-database calculations: aggregation pipelines and mapreduce. The aggregation pipeline system provides better performance and more coherent interface. However, map-reduce operations provide some flexibility that is not presently available in the aggregation pipeline.

## Aggregate

The `aggregate`() method allows you to run an aggregation [pipeline](https://docs.mongodb.com/manual/reference/method/db.collection.aggregate/). For example the pipeline below calculates the total flights per carrier and the average distance:


```r
stats <- flt$aggregate(
  '[{"$group":{"_id":"$carrier", "count": {"$sum":1}, "average":{"$avg":"$distance"}}}]',
  options = '{"allowDiskUse":true}'
)
names(stats) <- c("carrier", "count", "average")
print(stats)
```
```{.outputcode}
#>    carrier count   average
#> 1       OO    32  500.8125
#> 2       F9   685 1620.0000
#> 3       YV   601  375.0333
#> 4       EV 54173  562.9917
#> 5       FL  3260  664.8294
#> 6       9E 18460  530.2358
#> 7       AS   714 2402.0000
#> 8       US 20536  553.4563
#> 9       MQ 26397  569.5327
#> 10      UA 58665 1529.1149
#> 11      DL 48110 1236.9012
#> 12      B6 54635 1068.6215
#> 13      VX  5162 2499.4822
#> 14      WN 12275  996.2691
#> 15      HA   342 4983.0000
#> 16      AA 32729 1340.2360
```

Let's make a pretty plot:


```r
library(ggplot2)
ggplot(aes(carrier, count), data = stats) + geom_col()
```

<img src="mongobook_files/figure-html/figstats-1.png" width="672" />

Check the [MongoDB manual](https://docs.mongodb.com/manual/reference/method/db.collection.aggregate/#db.collection.aggregate) for detailed description of the pipeline syntax and supported options. 

## Map/Reduce

The `mapreduce()` method allow for running a custom in-database mapreduce job by providing custom `map` and `reduce` JavaScript functions. Running JavaScript is slower using the aggregate system, but you can implement fully customized database operations.

Below is a simple example to do "binning" of distances to create a histogram.


```r
# Map-reduce (binning)
histdata <- flt$mapreduce(
  map = "function(){emit(Math.floor(this.distance/100)*100, 1)}", 
  reduce = "function(id, counts){return Array.sum(counts)}"
)
names(histdata) <- c("distance", "count")
print(histdata)
```
```{.outputcode}
#>    distance count
#> 1         0  1633
#> 2       100 16017
#> 3       200 33637
#> 4       300  7748
#> 5       400 21182
#> 6       500 26925
#> 7       600  7846
#> 8       700 48904
#> 9       800  7574
#> 10      900 18205
#> 11     1000 49327
#> 12     1100  6336
#> 13     1200   332
#> 14     1300  9084
#> 15     1400  9313
#> 16     1500  8773
#> 17     1600  9220
#> 18     1700   243
#> 19     1800   315
#> 20     1900  2467
#> 21     2100  4656
#> 22     2200  5997
#> 23     2300    19
#> 24     2400 26052
#> 25     2500 14256
#> 26     3300     8
#> 27     4900   707
```

From this data we can create a pretty histogram:


```r
library(ggplot2)
ggplot(aes(distance, count), data = histdata) + geom_col()
```

<img src="mongobook_files/figure-html/fighist-1.png" width="672" />

Obviously we could have done binning in R instead, however if we are dealing with loads of data, doing it in database can be much more efficient. 










<!--chapter:end:05-calculation.Rmd-->

