#include <Rinternals.h>

SEXP C_join_pairlist(SEXP x, SEXP y) {
  if(!isPairList(x) || !isPairList(y))
    Rf_error("x and y must be pairlists");

  //special case
  if(x == R_NilValue)
    return y;

  //find the tail of x
  SEXP tail = getAttrib(x, install("tail"));
  if(tail == R_NilValue)
    tail = x;

  while(CDR(tail) != R_NilValue)
    tail = CDR(tail);

  //store tail
  setAttrib(x, install("tail"), y);

  //append to tail
  SETCDR(tail, y);
  return x;
}
