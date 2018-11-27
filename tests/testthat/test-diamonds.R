context("diamonds")
data(diamonds, package = "ggplot2")

m <- mongo("test_diamonds", verbose = FALSE)
if(m$count()) m$drop()
m2 <- mongo("test_diamonds_indices", verbose = FALSE)

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

test_that("one can create unique index for single field", {
  if (m2$count()) m2$drop()

  diamonds2 <- diamonds[1:100, ]
  diamonds2$simpleId <- 1:nrow(diamonds2)
  resL <- m2$insert(diamonds2)
  expect_equal(resL$nInserted, 100L)
  expect_equal(list(), resL$writeErrors)
  expect_equal(m2$count(),100L)

  simpleIdJson <- toJSON(list("simpleId" = 1), auto_unbox = TRUE)
  idxDf <- m2$index(add = simpleIdJson, addOpts = list(unique = TRUE))
  expect_true(idxDf[idxDf$name == "simpleId_1", "unique"], info = "index created successfully")

  expect_error(m2$insert(diamonds2[1,]), "E11000 duplicate key error collection")

  myV <- c(rep(1,10),101)
  names(myV) <- colnames(diamonds2)
  resL <- m2$insert(data.frame(t(myV)))
  expect_equal(resL$nInserted, 1L)
  expect_equal(list(), resL$writeErrors)
  expect_equal(m2$count(),101L)

})

test_that("one can create complex unique index", {
  if (m2$count()) m2$drop()

  diamonds2 <- diamonds[1:100, ]
  diamonds2$simpleId <- 1:nrow(diamonds2)
  diamonds3 <-
    cbind(diamonds2,
          secondId = rep(1:25, 4),
          thirdId = rep(1:4, each = 25))
  resL <- m2$insert(diamonds3)
  expect_equal(resL$nInserted, 100L)
  expect_equal(list(), resL$writeErrors)
  expect_equal(m2$count(),100L)

  complexIdxJson <- toJSON(list("secondId" = 1, "thirdId" = 1), auto_unbox = TRUE)
  idxDf <- m2$index(add = complexIdxJson, addOpts = list(unique = TRUE))
  expect_true(idxDf[idxDf$name == "secondId_1_thirdId_1", "unique"], info = "index created successfully")

  expect_error(m2$insert(diamonds3[1,]), "E11000 duplicate key error collection")

  myV <- c(rep(1, 10), c(100, 25, 4))
  names(myV) <- colnames(diamonds3)
  expect_error(
    m2$insert(data.frame(t(myV))),
    "E11000 duplicate key error collection.+secondId_1_thirdId_1 dup key"
  )

  myV <- c(rep(1, 10), c(100, 25, 5))
  names(myV) <- colnames(diamonds3)
  resL <- m2$insert(data.frame(t(myV)))
  expect_equal(resL$nInserted, 1L)
  expect_equal(list(), resL$writeErrors)
  expect_equal(m2$count(),101L)
})
