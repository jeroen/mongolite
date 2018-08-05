#include <mongolite.h>

//globals
static int bigint_as_char = 0;
static int date_as_char = 0;

SEXP ConvertArray(bson_iter_t* iter, bson_iter_t* counter);
SEXP ConvertObject(bson_iter_t* iter, bson_iter_t* counter);
SEXP ConvertValue(bson_iter_t* iter);
SEXP ConvertBinary(bson_iter_t* iter);
SEXP ConvertDate(bson_iter_t* iter);
SEXP ConvertDec128(bson_iter_t* iter);
SEXP ConvertTimestamp(bson_iter_t* iter);

SEXP R_bigint_as_char(SEXP x){
  if(Rf_isLogical(x))
    bigint_as_char = Rf_asLogical(x);
  return  Rf_ScalarLogical(bigint_as_char);
}

SEXP R_date_as_char(SEXP x){
  if(Rf_isLogical(x))
    date_as_char = Rf_asLogical(x);
  return Rf_ScalarLogical(date_as_char);
}

SEXP R_json_to_bson(SEXP json){
  bson_t *b;
  bson_error_t err;

  b = bson_new_from_json ((uint8_t*)  Rf_translateCharUTF8(Rf_asChar(json)), -1, &err);
  if(!b)
    stop(err.message);

  return bson2r(b);
}

SEXP R_raw_to_bson(SEXP buf){
  bson_error_t err;
  bson_t *b = bson_new_from_data(RAW(buf), Rf_length(buf));
  if(!b)
    stop(err.message);
  return bson2r(b);
}

SEXP R_bson_to_json(SEXP ptr){
  return mkStringUTF8(bson_as_relaxed_extended_json (r2bson(ptr), NULL));
}

SEXP R_bson_to_raw(SEXP ptr){
  bson_t *b = r2bson(ptr);
  const uint8_t *buf = bson_get_data(b);
  return mkRaw(buf, b->len);
}

SEXP R_bson_to_list(SEXP ptr) {
  bson_t *b = r2bson(ptr);
  return bson2list(b);
}

SEXP ConvertValue(bson_iter_t* iter){
  if(BSON_ITER_HOLDS_INT32(iter)){
    int res = bson_iter_int32(iter);
    return res == NA_INTEGER ? Rf_ScalarReal(res) : Rf_ScalarInteger(res);
  } else if(BSON_ITER_HOLDS_NULL(iter)){
    return R_NilValue;
  } else if(BSON_ITER_HOLDS_BOOL(iter)){
    return Rf_ScalarLogical(bson_iter_bool(iter));
  } else if(BSON_ITER_HOLDS_DOUBLE(iter)){
    return Rf_ScalarReal(bson_iter_double(iter));
  } else if(BSON_ITER_HOLDS_INT64(iter)){
    if(bigint_as_char){
      char buf[32];
      long long int x = bson_iter_int64(iter); //cross platform size
      snprintf(buf, 32, "%lld", x);
      return Rf_mkString(buf);
    } else {
      return Rf_ScalarReal((double) bson_iter_int64(iter));
    }
  } else if(BSON_ITER_HOLDS_UTF8(iter)){
    return mkStringUTF8(bson_iter_utf8(iter, NULL));
  } else if(BSON_ITER_HOLDS_CODE(iter)){
    return mkStringUTF8(bson_iter_code(iter, NULL));
  } else if(BSON_ITER_HOLDS_BINARY(iter)){
    return ConvertBinary(iter);
  } else if(BSON_ITER_HOLDS_DATE_TIME(iter)){
    return ConvertDate(iter);
  } else if(BSON_ITER_HOLDS_DECIMAL128(iter)){
    return ConvertDec128(iter);
  } else if(BSON_ITER_HOLDS_TIMESTAMP(iter)){
    return ConvertTimestamp(iter);
  } else if(BSON_ITER_HOLDS_OID(iter)){
    const bson_oid_t *val = bson_iter_oid(iter);
    char str[25];
    bson_oid_to_string(val, str);
    return Rf_mkString(str);
  } else if(BSON_ITER_HOLDS_SYMBOL(iter)){
    return mkStringUTF8(bson_iter_symbol(iter, NULL));
  } else if(BSON_ITER_HOLDS_ARRAY(iter)){
    bson_iter_t child1;
    bson_iter_t child2;
    bson_iter_recurse (iter, &child1);
    bson_iter_recurse (iter, &child2);
    return ConvertArray(&child1, &child2);
  } else if(BSON_ITER_HOLDS_DOCUMENT(iter)){
    bson_iter_t child1;
    bson_iter_t child2;
    bson_iter_recurse (iter, &child1);
    bson_iter_recurse (iter, &child2);
    return ConvertObject(&child1, &child2);
  } else {
    stop("Unimplemented BSON type %d\n", bson_iter_type(iter));
  }
}

SEXP ConvertTimestamp(bson_iter_t* iter){
  uint32_t timestamp;
  uint32_t increment;
  bson_iter_timestamp(iter, &timestamp, &increment);
  SEXP res = PROTECT(Rf_allocVector(VECSXP, 2));
  SET_VECTOR_ELT(res, 0, Rf_ScalarInteger(timestamp));
  SET_VECTOR_ELT(res, 1, Rf_ScalarInteger(increment));
  SEXP names = PROTECT(Rf_allocVector(STRSXP, 2));
  SET_STRING_ELT(names, 0, Rf_mkChar("t"));
  SET_STRING_ELT(names, 1, Rf_mkChar("i"));
  Rf_setAttrib(res, R_NamesSymbol, names);
  UNPROTECT(2);
  return res;
}
/*
SEXP ConvertDate(bson_iter_t* iter){
  SEXP list = PROTECT(allocVector(VECSXP, 1));
  SET_VECTOR_ELT(list, 0, ScalarReal((double) bson_iter_date_time(iter)));
  setAttrib(list, R_NamesSymbol, mkString("$date"));
  UNPROTECT(1);
  return list;
}
*/

SEXP ConvertDate(bson_iter_t* iter){
  if(date_as_char) {
    int64_t epoch = bson_iter_date_time(iter);
    int ms = epoch % 1000;
    time_t secs = epoch / 1000; //coerce int64 to int32
    struct tm * time = gmtime(&secs);
    char tmbuf[64], buf[70];
    strftime(tmbuf, sizeof tmbuf, "%Y-%m-%dT%H:%M:%S", time);
    snprintf(buf, sizeof buf, "%s.%03dZ", tmbuf, ms);
    return Rf_mkString(buf);
  }
  SEXP classes = PROTECT(Rf_allocVector(STRSXP, 2));
  SET_STRING_ELT(classes, 0, Rf_mkChar("POSIXct"));
  SET_STRING_ELT(classes, 1, Rf_mkChar("POSIXt"));
  //use 1000.0 to force cast to double!
  SEXP out = PROTECT(Rf_ScalarReal(bson_iter_date_time(iter) / 1000.0));
  Rf_setAttrib(out, R_ClassSymbol, classes);
  UNPROTECT(2);
  return out;
}

SEXP ConvertDec128(bson_iter_t* iter){
  bson_decimal128_t decimal128;
  bson_iter_decimal128(iter, &decimal128);
  char string[BSON_DECIMAL128_STRING];
  bson_decimal128_to_string (&decimal128, string);
  return Rf_ScalarReal(strtod(string, NULL));
}

SEXP ConvertBinary(bson_iter_t* iter){
  bson_subtype_t subtype;
  uint32_t binary_len;
  const uint8_t *binary;
  bson_iter_binary(iter, &subtype, &binary_len, &binary);

  //create raw vector
  SEXP out = PROTECT(Rf_allocVector(RAWSXP, binary_len));
  for (int i = 0; i < binary_len; i++) {
    RAW(out)[i] = binary[i];
  }
  Rf_setAttrib(out, PROTECT(Rf_install("type")), PROTECT(Rf_ScalarRaw(subtype)));
  UNPROTECT(3);
  return out;

}

SEXP ConvertArray(bson_iter_t* iter, bson_iter_t* counter){
  SEXP ret;
  int count = 0;
  while(bson_iter_next(counter)){
    count++;
  }
  PROTECT(ret = Rf_allocVector(VECSXP, count));
  for (int i = 0; bson_iter_next(iter); i++) {
    SET_VECTOR_ELT(ret, i, ConvertValue(iter));
  }
  UNPROTECT(1);
  return ret;
}

SEXP ConvertObject(bson_iter_t* iter, bson_iter_t* counter){
  SEXP names;
  SEXP ret;
  int count = 0;
  while(bson_iter_next(counter)){
    count++;
  }
  PROTECT(ret = Rf_allocVector(VECSXP, count));
  PROTECT(names = Rf_allocVector(STRSXP, count));
  for (int i = 0; bson_iter_next(iter); i++) {
    SET_STRING_ELT(names, i, Rf_mkChar(bson_iter_key(iter)));
    SET_VECTOR_ELT(ret, i, ConvertValue(iter));
  }
  Rf_setAttrib(ret, R_NamesSymbol, names);
  UNPROTECT(2);
  return ret;
}
