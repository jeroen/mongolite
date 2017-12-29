# We need https
if(getRversion() < "3.3.0") setInternet2()

# Download OpenSSL
if(!file.exists("../windows/openssl-1.1.0f/include/openssl/ssl.h")){
  download.file("https://github.com/rwinlib/openssl/archive/v1.1.0f.zip", "lib.zip", quiet = TRUE)
  dir.create("../windows", showWarnings = FALSE)
  unzip("lib.zip", exdir = "../windows")
  unlink("lib.zip")
}

