#include <mongolite.h>

SEXP ConvertArray(bson_iter_t* iter, bson_iter_t* counter);
SEXP ConvertObject(bson_iter_t* iter, bson_iter_t* counter);
SEXP ConvertValue(bson_iter_t* iter);
SEXP ConvertBinary(bson_iter_t* iter);
SEXP ConvertDate(bson_iter_t* iter);

SEXP R_json_to_bson(SEXP json){
  bson_t *b;
  bson_error_t err;

  b = bson_new_from_json ((uint8_t*)translateCharUTF8(asChar(json)), -1, &err);
  if(!b)
    stop(err.message);

  return bson2r(b);
}

SEXP R_raw_to_bson(SEXP buf){
  bson_t *b;
  bson_error_t err;
  int len = length(buf);
  uint8_t data[len];

  for (int i = 0; i < len; i++) {
    data[i] = RAW(buf)[i];
  }

  b = bson_new_from_data(data, len);
  if(!b)
    stop(err.message);

  return bson2r(b);
}

SEXP R_bson_to_json(SEXP ptr){
  return mkStringUTF8(bson_as_json (r2bson(ptr), NULL));
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
    return ScalarInteger(bson_iter_int32(iter));
  } else if(BSON_ITER_HOLDS_NULL(iter)){
    return R_NilValue;
  } else if(BSON_ITER_HOLDS_BOOL(iter)){
    return ScalarLogical(bson_iter_bool(iter));
  } else if(BSON_ITER_HOLDS_DOUBLE(iter)){
    return ScalarReal(bson_iter_double(iter));
  } else if(BSON_ITER_HOLDS_INT64(iter)){
    return ScalarReal((double) bson_iter_int64(iter));
  } else if(BSON_ITER_HOLDS_UTF8(iter)){
    return mkStringUTF8(bson_iter_utf8(iter, NULL));
  } else if(BSON_ITER_HOLDS_CODE(iter)){
    return mkStringUTF8(bson_iter_code(iter, NULL));
  } else if(BSON_ITER_HOLDS_BINARY(iter)){
    return ConvertBinary(iter);
  } else if(BSON_ITER_HOLDS_DATE_TIME(iter)){
    return ConvertDate(iter);
  } else if(BSON_ITER_HOLDS_OID(iter)){
    //not sure if this casting works
    return mkRaw((unsigned char *) bson_iter_oid(iter), 12);
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

SEXP ConvertDate(bson_iter_t* iter){
  SEXP list = PROTECT(allocVector(VECSXP, 1));
  SET_VECTOR_ELT(list, 0, ScalarReal((double) bson_iter_date_time(iter)));
  setAttrib(list, R_NamesSymbol, mkString("$date"));
  UNPROTECT(1);
  return list;
}

SEXP ConvertBinary(bson_iter_t* iter){
  bson_subtype_t subtype;
  uint32_t binary_len;
  const uint8_t *binary;
  bson_iter_binary(iter, &subtype, &binary_len, &binary);

  //create raw vector
  SEXP out = PROTECT(allocVector(RAWSXP, binary_len));
  for (int i = 0; i < binary_len; i++) {
    RAW(out)[i] = binary[i];
  }
  setAttrib(out, install("subtype"), ScalarInteger(subtype));
  UNPROTECT(1);
  return out;

}

SEXP ConvertArray(bson_iter_t* iter, bson_iter_t* counter){
  SEXP ret;
  int count = 0;
  while(bson_iter_next(counter)){
    count++;
  }
  PROTECT(ret = allocVector(VECSXP, count));
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
  PROTECT(ret = allocVector(VECSXP, count));
  PROTECT(names = allocVector(STRSXP, count));
  for (int i = 0; bson_iter_next(iter); i++) {
    SET_STRING_ELT(names, i, mkChar(bson_iter_key(iter)));
    SET_VECTOR_ELT(ret, i, ConvertValue(iter));
  }
  setAttrib(ret, R_NamesSymbol, names);
  UNPROTECT(2);
  return ret;
}
