context("flights")
library(nycflights13)

m <- mongo("test_flights", verbose = FALSE)
if(m$count()) m$drop()

test_that("insert data", {
  m$insert(flights)
  expect_equal(m$count(), nrow(flights))
  jan1 <- m$find('{"month":1, "day":1}')
  expect_equal(m$count('{"month":1, "day":1}'), nrow(jan1))
})

test_that("remove data", {
  m$remove('{}')
  expect_equal(m$count(), 0L)
  expect_true(m$drop())
})
