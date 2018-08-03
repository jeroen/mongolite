context("diamonds")
data(diamonds, package = "ggplot2")

m <- mongo("test_diamonds", verbose = FALSE)
if(m$count()) m$drop()

test_that("insert data", {
  m$insert(diamonds)
  expect_equal(m$count(), nrow(diamonds))
})

test_that("find query", {
  out1 <- m$find('{"cut" : "Premium", "price" : { "$lt" : 1000 } }')
  out2 <- subset(diamonds, cut == "Premium" & price < 1000)
  expect_equal(nrow(out1), nrow(out2))
})

test_that("remove data", {
  m$remove('{"cut" : "Premium", "price" : { "$lt" : 1000 } }', just_one = TRUE)
  expect_equal(m$count(), nrow(diamonds)-1)
  m$remove('{"cut" : "Premium", "price" : { "$lt" : 1000 } }', just_one = TRUE)
  expect_equal(m$count(), nrow(diamonds)-2)
  m$remove('{"cut" : "Premium", "price" : { "$lt" : 1000 } }', just_one = FALSE)
  expect_equal(m$count(), nrow(diamonds) - 3200)
  m$remove('{}', just_one = FALSE)
  expect_equal(m$count(), 0L)
  expect_true(m$drop())
})
