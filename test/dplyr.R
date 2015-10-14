library(mongolite)
library(dplyr)
library(nycflights13)

# Init connection
m <- mongo("flights", verbose = FALSE)
m$insert(flights)
m$count()

### FILTER

# And
x1 <- dplyr::filter(flights, day == 1, month > 6)
x2 <- m$find('{"day":1, "month":{"$gt":6}}')
nrow(x1) == nrow(x2)

# Not equal to
x1 <- dplyr::filter(flights, origin != "JFK")
x2 <- m$find('{"origin":{"$ne":"JFK"}}')
nrow(x1) == nrow(x2)

# Or
x1 <- dplyr::filter(flights, hour == 24 | hour <= 2)
x2 <- m$find('{"$or":[ {"hour":24}, {"hour":{"$lte":2}} ]}')
nrow(x1) == nrow(x2)


### SLICE

# Records are actually not ordered...
x1 <- dplyr::slice(flights, 21:30)
x2 <- m$find(skip=20, limit = 10, sort = '{"_id":1}')
all.equal(as.data.frame(x1), x2)

# Arrange

