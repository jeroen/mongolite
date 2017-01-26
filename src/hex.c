#include <Rinternals.h>

SEXP R_parse_hex_string(SEXP string){
  const char * str = CHAR(STRING_ELT(string, 0));
  size_t outlen = Rf_length(STRING_ELT(string, 0)) / 2;
  SEXP out = allocVector(RAWSXP, outlen);
  unsigned char * val = RAW(out);
  for(int i = 0; i < outlen; i++) {
    sscanf(str, "%2hhx", val);
    val++;
    str++;
    str++;
  }
  return out;
}
