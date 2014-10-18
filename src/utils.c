#include <utils.h>

SEXP mkStringUTF8(const char* str){
  SEXP out = PROTECT(allocVector(STRSXP, 1));
  SET_STRING_ELT(out, 0, Rf_mkCharCE(str, CE_UTF8));
  UNPROTECT(1);
  return out;
}
