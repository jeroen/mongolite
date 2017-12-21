#include <mongolite.h>

/* not sure what the purpose of this is */
void bson_reader_finalize(void *handle){

}

ssize_t bson_reader_feed(void *handle, void *buf, size_t count){
  int err;
  SEXP *con = (SEXP*) handle;
  SEXP x = PROTECT(LCONS(ScalarInteger(count), R_NilValue));
  SEXP y = PROTECT(LCONS(mkString("raw"), x));
  SEXP z = PROTECT(LCONS(*con, y));
  SEXP readbin = PROTECT(install("readBin"));
  SEXP call = PROTECT(LCONS(readbin, z));
  SEXP res = PROTECT(R_tryEval(call, R_GlobalEnv, &err));

  // check if readBin succeeded
  if(err || TYPEOF(res) != RAWSXP)
    Rf_error("Mongo reader failed to read data from connection. (%d)", err);

  // Copy data into buf
  memcpy(buf, RAW(res), length(res));
  UNPROTECT(6);
  return Rf_length(res);
}

SEXP R_mongo_restore(SEXP con, SEXP ptr_col, SEXP verb) {
  bool verbose = asLogical(verb);
  mongoc_collection_t *col = r2col(ptr_col);
  bson_reader_t *reader = bson_reader_new_from_handle(&con, bson_reader_feed, bson_reader_finalize);
  mongoc_bulk_operation_t *bulk = NULL;

  const bson_t *b;
  bson_error_t err;
  int count = 0;
  int i = 0;
  bool done = false;
  bson_t reply;

  while(!done) {
    //note: default opts uses {ordered:true}
    bulk = mongoc_collection_create_bulk_operation_with_opts(col, NULL);
    for(i = 0; i < 1000; i++){
      if(!(b = bson_reader_read (reader, &done)))
        break;
      mongoc_bulk_operation_insert (bulk, b);
      count++;
    }

    if(i == 0)
      break;

    if(!mongoc_bulk_operation_execute (bulk, &reply, &err)){
      bson_reader_destroy(reader);
      mongoc_bulk_operation_destroy (bulk);
      error(err.message);
    }

    if(verbose)
      Rprintf("\rRestored %d records...", count);
  }

  if(verbose)
    Rprintf("\rDone! Inserted total of %d records.\n", count);

  if (!done)
    warning("Failed to read all documents.\n");

  bson_reader_destroy(reader);
  mongoc_bulk_operation_destroy (bulk);
  return ScalarInteger(count);
}
