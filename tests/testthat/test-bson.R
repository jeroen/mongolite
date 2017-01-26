context("specifications")

roundtrip_json <- function(testdata){
  m <- mongo(verbose = FALSE)
  on.exit(try(m$drop(), silent = TRUE))
  extjson <- na.omit(testdata$valid$extjson)
  m$insert(extjson)
  out <- m$find()
  jsonlite::toJSON(out, raw = "mongo", collapse = FALSE, always_decimal = TRUE, digits = NA, na = "string", POSIXt = "mongo")
}

roundtrip_test <- function(testdata){
  json <- roundtrip_json(testdata)
  for(i in which(!is.na(testdata$valid$extjson))){
    x <- jsonlite::fromJSON(testdata$valid$extjson[i], simplifyVector = T)
    y <- jsonlite::fromJSON(json[i], simplifyVector = T)
    expect_equal(x, y)
  }
}

iterate_test <- function(testdata){
  m2 <- mongo(verbose = FALSE)
  on.exit(try(m2$drop(), silent = TRUE))
  lapply(testdata$valid$bson, function(bin){
    bson <- mongolite:::raw_to_bson(mongolite:::hex2bin(bin))
    m2$insert(bson)
  })
  # start iterator
  iter <- m2$iterate()
  invisible(lapply(testdata$valid$bson, function(bin){
    bson <- mongolite:::raw_to_bson(mongolite:::hex2bin(bin))
    list <- mongolite:::bson_to_list(bson)
    expect_equal(list, iter$one())
  }))
}

parse_number <- function(x){
  x <- sub("nan", "NaN", tolower(x), ignore.case = TRUE)
  x <- sub("inf[a-z]*", "Inf", x, ignore.case = TRUE)
  eval(parse(text=x))
}

test_that("roundtrip array", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/array.json")
  iterate_test(testdata)
  roundtrip_test(testdata)
})

test_that("roundtrip binary", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/binary.json")
  iterate_test(testdata)
  roundtrip_test(testdata)
})

test_that("roundtrip boolean", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/boolean.json")
  iterate_test(testdata)
  roundtrip_test(testdata)
})

test_that("roundtrip datetime", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/datetime.json")
  iterate_test(testdata)

  # Temporary workaround for date parsing bug in mongo-c-driver 1.5.x
  testdata$valid$extjson <- sub("1960", "1980", testdata$valid$extjson)
  roundtrip_test(testdata)
})

test_that("roundtrip document", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/document.json")
  iterate_test(testdata)

  # Remove test with empty column name.
  testdata$valid = testdata$valid[-2:-3,]
  roundtrip_test(testdata)
})

test_that("roundtrip double", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/double.json")
  iterate_test(testdata)

  roundtrip_test(testdata)
})

test_that("roundtrip int32", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/int32.json")
  iterate_test(testdata)
  roundtrip_test(testdata)
})

test_that("roundtrip string", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/string.json")
  iterate_test(testdata)
  roundtrip_test(testdata)
})

test_that("roundtrip timestamp", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/timestamp.json")
  iterate_test(testdata)

  # test only timestamp
  a <- jsonlite::fromJSON(testdata$valid$extjson)$a[["$timestamp"]]
  b <- jsonlite::fromJSON(roundtrip_json(testdata))$a
  expect_equal(a, b)
})

test_that("roundtrip dec128", {
  for(i in 1:5){
    testdata <- jsonlite::fromJSON(sprintf("specifications/source/bson-decimal128/tests/decimal128-%d.json", i))
    json <- roundtrip_json(testdata)
    for(i in seq_along(json)){
      x <- jsonlite::fromJSON(testdata$valid$extjson[i], simplifyVector = FALSE)$d[["$numberDecimal"]]
      y <- jsonlite::fromJSON(json[i], simplifyVector = FALSE)$d
      expect_equal(parse_number(x), parse_number(y))
    }
    iterate_test(testdata)
  }
})

