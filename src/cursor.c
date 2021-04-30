#include <mongolite.h>

SEXP R_mongo_cursor_more (SEXP ptr){
  mongoc_cursor_t *c = r2cursor(ptr);
  return Rf_ScalarLogical(mongoc_cursor_more(c));
}

SEXP R_mongo_cursor_next_bson (SEXP ptr){
  mongoc_cursor_t *c = r2cursor(ptr);
  const bson_t *b = NULL;
  if(!mongoc_cursor_next(c, &b)){
    bson_error_t err;
    if(mongoc_cursor_error (c, &err))
      stop(err.message);
    else
      return R_NilValue;
  }
  return bson2r((bson_t*) b);
}

SEXP R_mongo_cursor_next_bsonlist (SEXP ptr, SEXP n){
  mongoc_cursor_t *c = r2cursor(ptr);
  int len = Rf_asInteger(n);
  SEXP out = PROTECT(Rf_allocVector(VECSXP, len));
  const bson_t *b = NULL;
  int total = 0;
  bson_error_t err;
  while(total < len){
    if(!mongoc_cursor_next(c, &b)){
      if(mongoc_cursor_error (c, &err))
        stop(err.message);
      else
        break; //cursor exchausted: done
    } else {
      SEXP bin = PROTECT(Rf_allocVector(RAWSXP, b->len));
      memcpy(RAW(bin), bson_get_data(b), b->len);
      SET_VECTOR_ELT(out, total, bin);
      UNPROTECT(1);
      total++;
    }
  }
  if(total < len){
    SEXP out2 = PROTECT(Rf_allocVector(VECSXP, total));
    for(int i = 0; i < total; i++){
      SET_VECTOR_ELT(out2, i, VECTOR_ELT(out, i));
    }
    UNPROTECT(2);
    return out2;
  }
  UNPROTECT(1);
  return out;
}

SEXP R_mongo_cursor_next_json (SEXP ptr, SEXP n){
  mongoc_cursor_t *c = r2cursor(ptr);
  int len = Rf_asInteger(n);
  SEXP out = PROTECT(Rf_allocVector(STRSXP, len));
  const bson_t *b = NULL;
  int total = 0;
  bson_error_t err;
  while(total < len){
    if(!mongoc_cursor_next(c, &b)){
      if(mongoc_cursor_error (c, &err))
        stop(err.message);
      else
        //cursor exchausted: done
        break;
    } else {
      size_t jsonlength;
      char *str = bson_as_relaxed_extended_json ((bson_t*) b, &jsonlength);
      SET_STRING_ELT(out, total, Rf_mkCharLenCE(str, jsonlength, CE_UTF8));
      if(str) bson_free(str);
      total++;
    }
  }
  if(total < len){
    SEXP out2 = PROTECT(Rf_allocVector(STRSXP, total));
    for(int i = 0; i < total; i++){
      SET_STRING_ELT(out2, i, STRING_ELT(out, i));
    }
    UNPROTECT(2);
    return out2;
  }
  UNPROTECT(1);
  return out;
}


SEXP R_mongo_cursor_next_page(SEXP ptr, SEXP size, SEXP as_json){
  bson_error_t err;
  mongoc_cursor_t *c = r2cursor(ptr);
  int n = Rf_asInteger(size);
  const bson_t *b = NULL;
  SEXP list = PROTECT(Rf_allocVector(VECSXP, n));
  int total = 0;
  for(int i = 0; i < n && mongoc_cursor_next(c, &b); i++){
    if(Rf_asLogical(as_json)){
      SET_VECTOR_ELT(list, i, bson_to_str(b));
    } else {
      SET_VECTOR_ELT(list, i, bson2list((bson_t*) b));
    }
    total++;
  }

  //iterator exhausted
  if(total == 0){
    if(mongoc_cursor_error (c, &err))
      stop(err.message);
    UNPROTECT(1);
    return R_NilValue;
  }

  //found a full page
  if(total == n){
    UNPROTECT(1);
    return list;
  }

  //not a full page
  SEXP shortlist = PROTECT(Rf_allocVector(VECSXP, total));
  for(int i = 0; i < total; i++){
    SET_VECTOR_ELT(shortlist, i, VECTOR_ELT(list, i));
  }

  //also check for errors
  if(mongoc_cursor_error (c, &err))
    stop(err.message);

  UNPROTECT(2);
  return shortlist;
}

SEXP alloc_for_bson_type(bson_iter_t *iter, int n);
SEXP insert_bson_type(SEXP vec, bson_iter_t *iter, int row);
void fill_as_missing(SEXP vec, int row);
SEXP add_new_column(SEXP list, bson_iter_t *iter, int index, int n, int na_rows);
SEXP shrink_columns(SEXP list, int new_rows);
SEXP ConvertArray(bson_iter_t* iter, bson_iter_t* counter);

void R_heterogenous_column_error(bson_iter_t *iter, SEXP col, int row);
char* bson_type2char(bson_type_t t);

// Attribute name for binary subtypes.
static SEXP SubtypeSymbol = NULL;

SEXP R_mongo_cursor_next_page_flattened(SEXP ptr, SEXP size) {
  mongoc_cursor_t *c = r2cursor(ptr);
  int n = Rf_asInteger(size);
  const bson_t *b = NULL;
  bson_iter_t iter;
  int total = 0;
  SEXP names, ret;

  // Nothing here.
  if (!mongoc_cursor_next(c, &b)) {
    return R_NilValue;
  }

  bson_iter_init(&iter, (bson_t*) b);
  int cols = 0;
  while (bson_iter_next(&iter)) {
    cols++;
  }
  // We might need to resize, so keep track of the indices.
  PROTECT_INDEX names_index, ret_index, col_index;
  PROTECT_WITH_INDEX(names = Rf_allocVector(STRSXP, cols), &names_index);
  PROTECT_WITH_INDEX(ret = Rf_allocVector(VECSXP, cols), &ret_index);

  // Use the first entry to guess column names and types; later adjustments will
  // trigger reallocations of the list itself or coercion of column vectors.
  // Hopefully records are consistent enough that this only happens a few times.
  bson_iter_init(&iter, (bson_t*) b);
  SEXP col;
  for (int i = 0; bson_iter_next(&iter); i++) {
    SET_STRING_ELT(names, i, Rf_mkChar(bson_iter_key_unsafe(&iter)));
    PROTECT_WITH_INDEX(col = alloc_for_bson_type(&iter, n), &col_index);
    REPROTECT(col = insert_bson_type(col, &iter, 0), col_index);
    SET_VECTOR_ELT(ret, i, col);
    UNPROTECT(1);
  }
  total++;
  Rf_setAttrib(ret, R_NamesSymbol, names);

  int i;
  for (int row = 1; row < n && mongoc_cursor_next(c, &b); row++) {
    bson_iter_init(&iter, (bson_t *) b);

    // We can't simply loop over the bson entries here, because there might be
    // (1) more entries than we have columns, which requires the addition of a
    // new column; or (2) fewer entries than we have columns, which requires
    // that the remaining columns be filled with NAs.
    //
    // We do rely on the order of bson entries matching the order of existing
    // columns, which appears to be how mongo-c returns them to us.
    i = 0;
    while (true) {
      bool has_next = bson_iter_next(&iter);
      if (!has_next) {
        // Fill any remaining columns with NAs.
        for (i = i; i < cols; i++) {
          col = VECTOR_ELT(ret, i);
          fill_as_missing(col, row);
        }
        break;
      }
      if (i >= cols) { // Yet i > cols should never happen.
        REPROTECT(ret = add_new_column(ret, &iter, i, n, row), ret_index);
        REPROTECT(names = Rf_getAttrib(ret, R_NamesSymbol), names_index);
        cols++;
      }
      // Check that bson keys match the expected column name.
      if (strcmp(CHAR(STRING_ELT(names, i)), bson_iter_key_unsafe(&iter)) != 0) {
        int missing_column = 0, j = i;
        while (j < cols) {
          if (strcmp(CHAR(STRING_ELT(names, j)), bson_iter_key_unsafe(&iter)) == 0) {
            missing_column = j;
            break;
          }
          j++;
        }
        if (missing_column > 0) {
          // Fill in NAs and advance the index to that column before continuing.
          for (i = i; i < missing_column; i++) {
            col = VECTOR_ELT(ret, i);
            fill_as_missing(col, row);
          }
        } else {
          REPROTECT(ret = add_new_column(ret, &iter, i, n, row), ret_index);
          REPROTECT(names = Rf_getAttrib(ret, R_NamesSymbol), names_index);
          cols++;
        }
      }
      PROTECT_WITH_INDEX(col = VECTOR_ELT(ret, i), &col_index);
      REPROTECT(col = insert_bson_type(col, &iter, row), col_index);
      SET_VECTOR_ELT(ret, i, col);
      UNPROTECT(1);
      i++;
    }
    total++;
  }

  if (total != n) {
    ret = shrink_columns(ret, total);
  }

  bson_error_t err;
  if (mongoc_cursor_error(c, &err)) {
    stop(err.message);
  }

  UNPROTECT(2);
  return ret;
}

SEXP alloc_for_bson_type(bson_iter_t *iter, int n) {
  SEXP vec;
  switch (bson_iter_type(iter)) {
  case BSON_TYPE_DOUBLE: {
    vec = PROTECT(Rf_allocVector(REALSXP, n));
    break;
  }
  case BSON_TYPE_UTF8:
    // Fallthrough.
  case BSON_TYPE_OID:
    // Fallthrough.
  case BSON_TYPE_CODE:
    // Fallthrough.
  case BSON_TYPE_SYMBOL: {
    vec = PROTECT(Rf_allocVector(STRSXP, n));
    break;
  }
  case BSON_TYPE_DOCUMENT:
    // Fallthrough.
  case BSON_TYPE_ARRAY:
    // Fallthrough.
  case BSON_TYPE_TIMESTAMP: {
    vec = PROTECT(Rf_allocVector(VECSXP, n));
    break;
  }
  case BSON_TYPE_BINARY: {
    vec = PROTECT(Rf_allocVector(VECSXP, n));
    // Install the attribute to save time during decoding.
    if (SubtypeSymbol == NULL) {
      SubtypeSymbol = Rf_install("type");
    }
    break;
  }
  case BSON_TYPE_BOOL: {
    vec = PROTECT(Rf_allocVector(LGLSXP, n));
    break;
  }
  case BSON_TYPE_DATE_TIME: {
    SEXP classes = PROTECT(Rf_allocVector(STRSXP, 2));
    SET_STRING_ELT(classes, 0, Rf_mkChar("POSIXct"));
    SET_STRING_ELT(classes, 1, Rf_mkChar("POSIXt"));
    vec = PROTECT(Rf_allocVector(REALSXP, n));
    Rf_setAttrib(vec, R_ClassSymbol, classes);
    UNPROTECT(1);
    break;
  }
  case BSON_TYPE_NULL: {
    // For NULLs, take a similar approach to R: default to logical.
    Rf_warning("Found a null for column '%s' at row 1. Defaulting to logical \
vector.\n", bson_iter_key(iter));
    vec = PROTECT(Rf_allocVector(LGLSXP, n));
    break;
  }
  case BSON_TYPE_INT32: {
    vec = PROTECT(Rf_allocVector(INTSXP, n));
    break;
  }
  case BSON_TYPE_INT64:
    // Fallthrough.
  case BSON_TYPE_DECIMAL128: {
    vec = PROTECT(Rf_allocVector(REALSXP, n));
    break;
  }
  default:
    stop("Unsupported flat BSON type in column '%s': '%s'.\n",
         bson_iter_key(iter), bson_type2char(bson_iter_type(iter)));
  }
  UNPROTECT(1);
  return vec;
}

SEXP insert_bson_type(SEXP vec, bson_iter_t *iter, int row) {
  // We handle type safety and any needed type coercions here. For heterogenous
  // columns that can be transparently coerced (e.g. mixed numeric/logical data
  // or mixed numeric/string data), we do that here, following R's normal
  // coercion rules. Otherwise, we throw an error.
  switch (bson_iter_type(iter)) {
  case BSON_TYPE_DOUBLE: {
    if (Rf_isReal(vec)) {
      REAL(vec)[row] = bson_iter_double_unsafe(iter);
    } else if (Rf_isInteger(vec) || Rf_isLogical(vec)) {
      vec = PROTECT(Rf_coerceVector(vec, REALSXP));
      REAL(vec)[row] = bson_iter_double_unsafe(iter);
      UNPROTECT(1);
    } else if (Rf_isString(vec)) {
      SEXP elt = PROTECT(Rf_ScalarReal(bson_iter_double_unsafe(iter)));
      PROTECT(elt = Rf_asChar(elt));
      SET_STRING_ELT(vec, row, elt);
      UNPROTECT(2);
    } else {
      R_heterogenous_column_error(iter, vec, row);
    }
    break;
  }
  case BSON_TYPE_UTF8: {
    // Turn non-string columns into strings, if necessary.
    if (!Rf_isString(vec)) {
      vec = Rf_coerceVector(vec, STRSXP);
    }
    SET_STRING_ELT(vec, row, Rf_mkCharCE(bson_iter_utf8(iter, NULL), CE_UTF8));
    break;
  }
  case BSON_TYPE_DOCUMENT: {
    if (Rf_isVectorList(vec)) {
      bson_iter_t child1, child2;
      bson_iter_recurse(iter, &child1);
      bson_iter_recurse(iter, &child2);
      SEXP elt = PROTECT(ConvertObject(&child1, &child2));
      SET_VECTOR_ELT(vec, row, elt);
      UNPROTECT(1);
    } else {
      R_heterogenous_column_error(iter, vec, row);
    }
    break;
  }
  case BSON_TYPE_ARRAY: {
    if (Rf_isVectorList(vec)) {
      bson_iter_t child1, child2;
      bson_iter_recurse(iter, &child1);
      bson_iter_recurse(iter, &child2);
      SEXP elt = PROTECT(ConvertArray(&child1, &child2));
      SET_VECTOR_ELT(vec, row, elt);
      UNPROTECT(1);
    } else {
      R_heterogenous_column_error(iter, vec, row);
    }
    break;
  }
  case BSON_TYPE_BINARY: {
    if (Rf_isVectorList(vec)) {
      bson_subtype_t subtype;
      uint32_t binary_len;
      const uint8_t *binary;
      bson_iter_binary(iter, &subtype, &binary_len, &binary);

      SEXP elt = PROTECT(Rf_allocVector(RAWSXP, binary_len));
      Rbyte *raw = RAW(elt);
      for (int i = 0; i < binary_len; i++) {
        raw[i] = binary[i];
      }
      Rf_setAttrib(elt, SubtypeSymbol, PROTECT(Rf_ScalarRaw(subtype)));
      SET_VECTOR_ELT(vec, row, elt);
      UNPROTECT(2);
    } else {
      R_heterogenous_column_error(iter, vec, row);
    }
    break;
  }
  case BSON_TYPE_OID: {
    if (!Rf_isString(vec)) { // This seems unlikely to happen.
      R_heterogenous_column_error(iter, vec, row);
    }
    const bson_oid_t *val = bson_iter_oid(iter);
    char str[25];
    bson_oid_to_string(val, str);
    SET_STRING_ELT(vec, row, Rf_mkCharLenCE(str, 24, CE_NATIVE));
    break;
  }
  case BSON_TYPE_BOOL: {
    if (Rf_isLogical(vec)) {
      LOGICAL(vec)[row] = bson_iter_bool_unsafe(iter);
    } else if (Rf_isInteger(vec)) {
      INTEGER(vec)[row] = bson_iter_bool_unsafe(iter);
      UNPROTECT(1);
    } else if (Rf_isReal(vec)) {
      REAL(vec)[row] = (double) bson_iter_bool_unsafe(iter);
      UNPROTECT(1);
    } else if (Rf_isString(vec)) {
      SEXP elt = PROTECT(Rf_ScalarLogical(bson_iter_bool_unsafe(iter)));
      PROTECT(elt = Rf_asChar(elt));
      SET_STRING_ELT(vec, row, elt);
      UNPROTECT(2);
    } else {
      R_heterogenous_column_error(iter, vec, row);
    }
    break;
  }
  case BSON_TYPE_DATE_TIME: {
    if (Rf_isReal(vec)) {
      REAL(vec)[row] = bson_iter_date_time(iter) / 1000.0;
    } else {
      // TODO: Handle stringified date time data.
      R_heterogenous_column_error(iter, vec, row);
    }
    break;
  }
  case BSON_TYPE_NULL: {
    fill_as_missing(vec, row);
    break;
  }
  case BSON_TYPE_CODE: {
    if (!Rf_isString(vec)) {
      vec = Rf_coerceVector(vec, STRSXP);
    }
    SET_STRING_ELT(vec, row, Rf_mkCharCE(bson_iter_code(iter, NULL), CE_UTF8));
    break;
  }
  case BSON_TYPE_SYMBOL: {
    if (!Rf_isString(vec)) {
      vec = Rf_coerceVector(vec, STRSXP);
    }
    SET_STRING_ELT(vec, row, Rf_mkCharCE(bson_iter_symbol(iter, NULL), CE_UTF8));
    break;
  }
  case BSON_TYPE_INT32: {
    if (Rf_isInteger(vec)) {
      int raw = bson_iter_int32_unsafe(iter);
      if (raw == NA_INTEGER) {
        // This is an edge case. Handle it the same way that R does: convert to
        // numeric.
        vec = PROTECT(Rf_coerceVector(vec, REALSXP));
        REAL(vec)[row] = (double) raw;
        UNPROTECT(1);
      } else {
        INTEGER(vec)[row] = raw;
      }
    } else if (Rf_isReal(vec)) {
      int res = bson_iter_int32_unsafe(iter);
      REAL(vec)[row] = res == NA_INTEGER ? NA_REAL : (double) res;
    } else if (Rf_isString(vec)) {
      SEXP elt = PROTECT(Rf_ScalarInteger(bson_iter_int32_unsafe(iter)));
      PROTECT(elt = Rf_asChar(elt));
      SET_STRING_ELT(vec, row, elt);
      UNPROTECT(2);
    } else if (Rf_isLogical(vec)) {
      vec = PROTECT(Rf_coerceVector(vec, INTSXP));
      INTEGER(vec)[row] = bson_iter_int32_unsafe(iter);
      UNPROTECT(1);
    } else {
      R_heterogenous_column_error(iter, vec, row);
    }
    break;
  }
  case BSON_TYPE_INT64: {
    if (Rf_isReal(vec)) {
      REAL(vec)[row] = (double) bson_iter_int64_unsafe(iter);
    } else {
      // TODO: Handle alternative represenations.
      R_heterogenous_column_error(iter, vec, row);
    }
    break;
  }
  case BSON_TYPE_TIMESTAMP: {
    if (Rf_isVectorList(vec)) {
      uint32_t timestamp, increment;
      bson_iter_timestamp(iter, &timestamp, &increment);

      SEXP elt = PROTECT(Rf_allocVector(VECSXP, 2));
      SET_VECTOR_ELT(elt, 0, Rf_ScalarInteger(timestamp));
      SET_VECTOR_ELT(elt, 1, Rf_ScalarInteger(increment));

      SEXP names = PROTECT(Rf_allocVector(STRSXP, 2));
      SET_STRING_ELT(names, 0, Rf_mkChar("t"));
      SET_STRING_ELT(names, 1, Rf_mkChar("i"));
      Rf_setAttrib(elt, R_NamesSymbol, names);

      SET_VECTOR_ELT(vec, row, elt);
      UNPROTECT(2);
    } else {
      R_heterogenous_column_error(iter, vec, row);
    }
    break;
  }
  case BSON_TYPE_DECIMAL128: {
    if (Rf_isReal(vec)) {
      bson_decimal128_t decimal128;
      bson_iter_decimal128_unsafe(iter, &decimal128);
      char string[BSON_DECIMAL128_STRING];
      bson_decimal128_to_string (&decimal128, string);
      REAL(vec)[row] = strtod(string, NULL);
    } else {
      R_heterogenous_column_error(iter, vec, row);
    }
    break;
  }
  default:
    stop("Unsupported flat BSON type at row %d: '%s'.\n",
         row + 1, bson_type2char(bson_iter_type(iter)));
  }

  return vec;
}

void fill_as_missing(SEXP vec, int row) {
  switch(TYPEOF(vec)) {
  case LGLSXP:
    LOGICAL(vec)[row] = NA_LOGICAL;
    break;
  case INTSXP:
    INTEGER(vec)[row] = NA_INTEGER;
    break;
  case REALSXP:
    REAL(vec)[row] = NA_REAL;
    break;
  case STRSXP:
    SET_STRING_ELT(vec, row, NA_STRING);
    break;
  case VECSXP: {
    SET_VECTOR_ELT(vec, row, R_NilValue);
    break;
  }
  default:
    stop("Can't fill vector of type %d as missing.");
  }
}

SEXP add_new_column(SEXP list, bson_iter_t *iter, int index, int n, int na_rows) {
  int size = Rf_length(list), offset = 0;
  SEXP names = Rf_getAttrib(list, R_NamesSymbol);

  SEXP new_names = PROTECT(Rf_allocVector(STRSXP, size + 1));
  SET_STRING_ELT(new_names, index, Rf_mkChar(bson_iter_key(iter)));
  for (int i = 0; i < size; i++) {
    if (i == index) {
      offset = 1;
    }
    SET_STRING_ELT(new_names, i + offset, STRING_ELT(names, i));
  }

  // Fill all previous rows as missing.
  SEXP new_elt = PROTECT(alloc_for_bson_type(iter, n));
  switch(TYPEOF(new_elt)) {
  case LGLSXP: {
    int *raw = LOGICAL(new_elt);
    for (int i = 0; i < na_rows; i++) {
      raw[i] = NA_LOGICAL;
    }
    break;
  }
  case INTSXP: {
    int *raw = INTEGER(new_elt);
    for (int i = 0; i < na_rows; i++) {
      raw[i] = NA_INTEGER;
    }
    break;
  }
  case REALSXP: {
    double *raw = REAL(new_elt);
    for (int i = 0; i < na_rows; i++) {
      raw[i] = NA_REAL;
    }
    break;
  }
  case STRSXP: {
    for (int i = 0; i < na_rows; i++) {
      SET_STRING_ELT(new_elt, i, NA_STRING);
    }
    break;
  }
  case VECSXP: {
    for (int i = 0; i < na_rows; i++) {
      SET_VECTOR_ELT(new_elt, i, R_NilValue);
    }
    break;
  }
  default:
    stop("Can't fill vector of type %d as missing.");
  }

  SEXP new_list = PROTECT(Rf_allocVector(VECSXP, size + 1));
  SET_VECTOR_ELT(new_list, index, new_elt);
  offset = 0;
  for (int i = 0; i < size; i++) {
    if (i == index) {
      offset = 1;
    }
    SET_VECTOR_ELT(new_list, i + offset, VECTOR_ELT(list, i));
  }

  Rf_setAttrib(new_list, R_NamesSymbol, new_names);
  UNPROTECT(3);
  return new_list;
}

SEXP shrink_columns(SEXP list, int new_rows) {
  SEXP col, new_col;
  int size = Rf_length(list);

  for (int i = 0; i < size; i++) {
    col = VECTOR_ELT(list, i);
    switch(TYPEOF(col)) {
    case LGLSXP: {
      int *raw_col = LOGICAL(col);
      new_col = PROTECT(Rf_allocVector(LGLSXP, new_rows));
      int *raw_new_col = LOGICAL(new_col);
      for (int i = 0; i < new_rows; i++) {
        raw_new_col[i] = raw_col[i];
      }
      SET_VECTOR_ELT(list, i, new_col);
      UNPROTECT(1);
      break;
    }
    case INTSXP: {
      int *raw_col = INTEGER(col);
      new_col = PROTECT(Rf_allocVector(INTSXP, new_rows));
      int *raw_new_col = INTEGER(new_col);
      for (int i = 0; i < new_rows; i++) {
        raw_new_col[i] = raw_col[i];
      }
      SET_VECTOR_ELT(list, i, new_col);
      UNPROTECT(1);
      break;
    }
    case REALSXP: {
      double *raw_col = REAL(col);
      new_col = PROTECT(Rf_allocVector(REALSXP, new_rows));
      double *raw_new_col = REAL(new_col);
      for (int i = 0; i < new_rows; i++) {
        raw_new_col[i] = raw_col[i];
      }
      // Preserve class info (e.g. for date time data).
      Rf_setAttrib(new_col, R_ClassSymbol, Rf_getAttrib(col, R_ClassSymbol));
      SET_VECTOR_ELT(list, i, new_col);
      UNPROTECT(1);
      break;
    }
    case STRSXP: {
      new_col = PROTECT(Rf_allocVector(STRSXP, new_rows));
      for (int i = 0; i < new_rows; i++) {
        SET_STRING_ELT(new_col, i, STRING_ELT(col, i));
      }
      SET_VECTOR_ELT(list, i, new_col);
      UNPROTECT(1);
      break;
    }
    case VECSXP: {
      new_col = PROTECT(Rf_allocVector(VECSXP, new_rows));
      for (int i = 0; i < new_rows; i++) {
        SET_VECTOR_ELT(new_col, i, VECTOR_ELT(col, i));
      }
      SET_VECTOR_ELT(list, i, new_col);
      UNPROTECT(1);
      break;
    }
    default:
      stop("Can't resize vector of type '%s'.", Rf_type2char(TYPEOF(col)));
    }
  }

  return list;
}

char* bson_type2char(bson_type_t t) {
  char *b_type;
  switch (t) {
  case BSON_TYPE_DOUBLE:
    b_type = "double";
    break;
  case BSON_TYPE_UTF8:
    b_type = "utf-8";
    break;
  case BSON_TYPE_DOCUMENT:
    b_type = "document";
    break;
  case BSON_TYPE_ARRAY:
    b_type = "array";
    break;
  case BSON_TYPE_BINARY:
    b_type = "binary";
    break;
  case BSON_TYPE_OID:
    b_type = "oid";
    break;
  case BSON_TYPE_BOOL:
    b_type = "bool";
    break;
  case BSON_TYPE_DATE_TIME:
    b_type = "date-time";
    break;
  case BSON_TYPE_NULL:
    b_type = "null";
    break;
  case BSON_TYPE_CODE:
    b_type = "code";
    break;
  case BSON_TYPE_SYMBOL:
    b_type = "symbol";
    break;
  case BSON_TYPE_INT32:
    b_type = "int32";
    break;
  case BSON_TYPE_TIMESTAMP:
    b_type = "timestamp";
    break;
  case BSON_TYPE_INT64:
    b_type = "int64";
    break;
  case BSON_TYPE_DECIMAL128:
    b_type = "decimal 128";
    break;
  default: {
    static char buf[5];
    snprintf(buf, 5, "%d", t);
    b_type = buf;
  }
  }
  return b_type;
}

void R_heterogenous_column_error(bson_iter_t *iter, SEXP col, int row) {
  const char *r_type, *b_type = bson_type2char(bson_iter_type(iter));
  switch (TYPEOF(col)) {
  case LGLSXP:
    r_type = "logical";
    break;
  case INTSXP:
    r_type = "integer";
    break;
  case REALSXP:
    r_type = "numeric";
    break;
  case STRSXP:
    r_type = "character";
    break;
  default:
    r_type = Rf_type2char(TYPEOF(col));
  }
  stop("Unsupported heterogenous column '%s'. Found BSON '%s' type at row %d, \
which cannot be placed in an R %s vector.\n",
       bson_iter_key(iter), b_type, row + 1, r_type);
}
