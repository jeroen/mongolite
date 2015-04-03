# Run 'mongod' in the console to start MongoDB
# install_github("jeroenooms/mongolite")
library(mongolite)

# Init connection
m <- mongo_connect(collection = "cmdtest")
mongo_stream_out(iris, m)

# Run a command
mongo_collection_command(m, '{"collStats":"cmdtest"}')
