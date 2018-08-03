context("specifications")

roundtrip_json <- function(testdata){
  m <- mongo(verbose = FALSE)
  on.exit(try(m$drop(), silent = TRUE))
  extjson <- na.omit(testdata$valid$canonical_extjson)
  m$insert(extjson)
  out <- m$find()
  jsonlite::toJSON(out, raw = "mongo", collapse = FALSE, always_decimal = TRUE, digits = NA, na = "string", POSIXt = "mongo")
}

roundtrip_test <- function(testdata, spec_name) {
  json <- roundtrip_json(testdata)
  for(i in which(!is.na(testdata$valid$canonical_extjson))){
    x <- jsonlite::fromJSON(testdata$valid$canonical_extjson[i], simplifyVector = T)
    y <- jsonlite::fromJSON(json[i], simplifyVector = T)
    expect_equal(x, y, info = sprintf(
      "Error in JSON roundtrip for valid %s entry \"%s\" (%d).",
      spec_name, testdata$valid$description[i], i
    ))
  }
}

iterate_test <- function(testdata, spec_name) {
  m2 <- mongo(verbose = FALSE)
  on.exit(try(m2$drop(), silent = TRUE))
  lapply(testdata$valid$canonical_bson, function(bin){
    bson <- mongolite:::raw_to_bson(mongolite:::hex2bin(bin))
    m2$insert(bson)
  })
  # start iterator
  iter <- m2$iterate()
  for (i in 1:length(testdata$valid$canonical_bson)) {
    bin <- testdata$valid$canonical_bson[i]
    bson <- mongolite:::raw_to_bson(mongolite:::hex2bin(bin))
    list <- mongolite:::bson_to_list(bson)
    expect_equal(list, iter$one(), info = sprintf(
      "Error in BSON roundtrip for valid %s entry \"%s\" (%d).",
      spec_name, testdata$valid$description[i], i
    ))
  }
}

parse_number <- function(x){
  x <- sub("nan", "NaN", tolower(x), ignore.case = TRUE)
  x <- sub("inf[a-z]*", "Inf", x, ignore.case = TRUE)
  eval(parse(text=x))
}

test_that("roundtrip array", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/array.json")
  iterate_test(testdata, "array")

  # Avoid new canonical format issues by comparing elements.
  key <- testdata$test_key
  json <- roundtrip_json(testdata)
  for(i in which(!is.na(testdata$valid$canonical_extjson))) {
    x <- jsonlite::fromJSON(testdata$valid$canonical_extjson[i],
                            simplifyVector = TRUE)[[key]]
    if (length(x) > 0) x <- parse_number(x[[1]])
    y <- jsonlite::fromJSON(json[i], simplifyVector = TRUE)[[key]]
    if (length(y) > 0) y <- parse_number(y)
    expect_equal(x, y, info = sprintf(
      "Error in JSON roundtrip for valid %s entry \"%s\" (%d).",
      "array", testdata$valid$description[i], i
    ))
  }
})

test_that("roundtrip binary", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/binary.json")

  # Avoid running tests which fail due to unimplemented $type checking.
  testdata$valid <- testdata$valid[1:9,]
  iterate_test(testdata, "binary")

  # Avoid new canonical format by directly comparing base64-encoded values.
  # NOTE: This may change if `jsonlite`'s implementation changes. In that case,
  # we could fall back on using roundtrip_test().
  key <- testdata$test_key
  json <- roundtrip_json(testdata)
  for(i in which(!is.na(testdata$valid$canonical_extjson))) {
    x <- jsonlite::fromJSON(testdata$valid$canonical_extjson[i],
                            simplifyVector = TRUE)[[key]]
    x <- x$`$binary`$base64
    y <- jsonlite::fromJSON(json[i], simplifyVector = TRUE)[[key]]
    y <- y$`$binary`
    expect_equal(x, y, info = sprintf(
      "Error in JSON roundtrip for valid %s entry \"%s\" (%d).",
      "binary", testdata$valid$description[i], i
    ))
  }
})

test_that("roundtrip boolean", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/boolean.json")
  iterate_test(testdata, "boolean")
  roundtrip_test(testdata, "boolean")
})

test_that("roundtrip datetime", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/datetime.json")
  iterate_test(testdata, "datetime")

  # Avoid new canonical format by directly comparing numeric values.
  key <- testdata$test_key
  json <- roundtrip_json(testdata)
  for(i in which(!is.na(testdata$valid$canonical_extjson))) {
    x <- jsonlite::fromJSON(testdata$valid$canonical_extjson[i],
                            simplifyVector = TRUE)[[key]]
    x <- as.numeric(x$`$date`$`$numberLong`)
    # NOTE: No [[key]] here due to jsonlite's implementation.
    y <- jsonlite::fromJSON(json[i], simplifyVector = TRUE)
    y <- unclass(y) * 1000
    expect_equal(x, y, info = sprintf(
      "Error in JSON roundtrip for valid %s entry \"%s\" (%d).",
      "datetime", testdata$valid$description[i], i
    ))
  }
})

test_that("roundtrip document", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/document.json")

  # Remove test with empty column name.
  testdata$valid <- testdata$valid[-2,]
  iterate_test(testdata, "document")

  # Remove test with empty subdocument name.
  testdata$valid <- testdata$valid[-1,]
  roundtrip_test(testdata, "document")
})

test_that("roundtrip double", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/double.json")
  iterate_test(testdata, "double")

  # Avoid new canonical format by directly comparing numeric values.
  key <- testdata$test_key
  json <- roundtrip_json(testdata)
  for(i in which(!is.na(testdata$valid$canonical_extjson))) {
    x <- jsonlite::fromJSON(testdata$valid$canonical_extjson[i],
                            simplifyVector = TRUE)[[key]]
    x <- parse_number(x$`$numberDouble`)
    y <- jsonlite::fromJSON(json[i], simplifyVector = TRUE)[[key]]
    y <- parse_number(y)
    expect_equal(x, y, info = sprintf(
      "Error in JSON roundtrip for valid %s entry \"%s\" (%d).",
      "double", testdata$valid$description[i], i
    ))
  }
})

test_that("roundtrip int32", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/int32.json")
  iterate_test(testdata, "int32")

  # Avoid new canonical format by directly comparing numeric values.
  key <- testdata$test_key
  json <- roundtrip_json(testdata)
  for(i in which(!is.na(testdata$valid$canonical_extjson))) {
    x <- jsonlite::fromJSON(testdata$valid$canonical_extjson[i],
                            simplifyVector = TRUE)[[key]]
    y <- jsonlite::fromJSON(json[i], simplifyVector = TRUE)[[key]]
    x <- parse_number(x$`$numberInt`)
    y <- parse_number(y)
    expect_equal(x, y, info = sprintf(
      "Error in JSON roundtrip for valid %s entry \"%s\" (%d).",
      "int32", testdata$valid$description[i], i
    ))
  }
})

test_that("roundtrip string", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/string.json")
  iterate_test(testdata, "string")
  roundtrip_test(testdata, "string")
})

test_that("roundtrip timestamp", {
  testdata <- jsonlite::fromJSON("specifications/source/bson-corpus/tests/timestamp.json")
  iterate_test(testdata, "timestamp")

  # Avoid running the test with high-order bits.
  testdata$valid <- testdata$valid[-3,]

  # Avoid new canonical format by directly comparing numeric values.
  key <- testdata$test_key
  json <- roundtrip_json(testdata)
  for(i in which(!is.na(testdata$valid$canonical_extjson))) {
    x <- jsonlite::fromJSON(testdata$valid$canonical_extjson[i],
                            simplifyVector = TRUE)[[key]]
    y <- jsonlite::fromJSON(json[i], simplifyVector = TRUE)[[key]]
    x <- parse_number(x$`$timestamp`)
    y <- parse_number(y)
    expect_equal(x, y, info = sprintf(
      "Error in JSON roundtrip for valid %s entry \"%s\" (%d).",
      "timestamp", testdata$valid$description[i], i
    ))
  }
})

test_that("roundtrip dec128", {
  # Avoid running the tests with overflow/underflow values.
  for(i in 1:4) {
    testdata <- jsonlite::fromJSON(sprintf("specifications/source/bson-corpus/tests/decimal128-%d.json", i))
    json <- roundtrip_json(testdata)
    for(i in seq_along(json)){
      x <- jsonlite::fromJSON(testdata$valid$canonical_extjson[i], simplifyVector = FALSE)$d[["$numberDecimal"]]
      y <- jsonlite::fromJSON(json[i], simplifyVector = FALSE)$d
      expect_equal(parse_number(x), parse_number(y))
    }
    iterate_test(testdata, "dec128")
  }
})

