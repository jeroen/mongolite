# We need https
if(getRversion() < "3.3.0") setInternet2()

# Download OpenSSL
if(!file.exists("../windows/openssl-1.0.2d/include/openssl/ssl.h")){
  download.file("https://github.com/rwinlib/openssl/archive/v1.0.2d.zip", "lib.zip", quiet = TRUE)
  dir.create("../windows", showWarnings = FALSE)
  unzip("lib.zip", exdir = "../windows")
  unlink("lib.zip")
}

# Download SASL
if(!file.exists("../windows/libsasl-2.1.26/include/sasl/sasl.h")){
  download.file("https://github.com/rwinlib/libsasl/archive/v2.1.26.zip", "lib.zip", quiet = TRUE)
  dir.create("../windows", showWarnings = FALSE)
  unzip("lib.zip", exdir = "../windows")
  unlink("lib.zip")
}
