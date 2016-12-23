context("specifications")

roundtrip_json <- function(testdata){
  extjson <- na.omit(testdata$valid$extjson)
  m <- mongo(verbose = FALSE)
  on.exit(try(m$drop(), silent = TRUE))
  m$insert(extjson)
  out <- m$find()
  jsonlite::toJSON(out, raw = "mongo", collapse = FALSE, always_decimal = TRUE, digits = NA)
}

roundtrip_test <- function(testdata){
  json <- roundtrip_json(testdata)
  for(i in seq_along(json)){
    x <- jsonlite::fromJSON(testdata$valid$extjson[i], simplifyVector = FALSE)
    y <- jsonlite::fromJSON(json[i], simplifyVector = FALSE)
    expect_equal(x, y)
  }
}

parse_number <- function(x){
  x <- sub("nan", "NaN", tolower(x), ignore.case = TRUE)
  x <- sub("inf[a-z]*", "Inf", x, ignore.case = TRUE)
  eval(parse(text=x))
}

test_that("roundtrip array", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/array.json")
  roundtrip_test(testdata)
})

test_that("roundtrip binary", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/binary.json")
  roundtrip_test(testdata)
})

test_that("roundtrip boolean", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/boolean.json")
  roundtrip_test(testdata)
})

test_that("roundtrip datetime", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/datetime.json")
  roundtrip_test(testdata)
})

test_that("roundtrip document", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/document.json")
  # Remove test with empty column name.
  testdata$valid = testdata$valid[-2,]
  roundtrip_test(testdata)
})

test_that("roundtrip double", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/double.json")
  roundtrip_test(testdata)
})

test_that("roundtrip int32", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/int32.json")
  roundtrip_test(testdata)
})

test_that("roundtrip string", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/string.json")
  roundtrip_test(testdata)
})

test_that("roundtrip timestamp", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/timestamp.json")
  a <- jsonlite::fromJSON(testdata$valid$extjson)$a[["$timestamp"]]
  b <- jsonlite::fromJSON(roundtrip_json(testdata))$a
  expect_equal(a, b)
})

test_that("roundtrip dec128", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-decimal128/tests/decimal128-1.json")
  json <- roundtrip_json(testdata)
  for(i in seq_along(json)){
    x <- jsonlite::fromJSON(testdata$valid$extjson[i], simplifyVector = FALSE)$d[["$numberDecimal"]]
    y <- jsonlite::fromJSON(json[i], simplifyVector = FALSE)$d
    expect_equal(parse_number(x), parse_number(y))
  }
})

