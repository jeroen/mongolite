context("test-oid.R")

m <- mongo("test_oid", verbose = FALSE)
m$drop()

df <- data.frame(`_id` = I(create_bson_oid(10)), x = 1:10, check.names = FALSE)

test_that("data containing OIDs can be inserted", {
  ins <- m$insert(df)
  expect_equal(m$count(), nrow(df))
})

test_that("data containing OIDs retains those OIDs", {
  out <- m$find(fields = "{}")
  expect_true("_id" %in% colnames(out))
  expect_equal(out$`_id`, as.character(df$`_id`))
})
