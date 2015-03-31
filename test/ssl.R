library(mongolite)
m <- mongo_connect("mongodb://localhost/?ssl=true", collection = "ssltest")
mongo_stream_out(mtcars, m)
out <- mongo_stream_in(m)
