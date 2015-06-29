# This is a workaround until 'mongoc_client_get_default_database' becomes
# available in mongo-c-driver 1.2.0
get_path_from_url <- function(url){
  if(!grepl("^mongodb://", url))
    stop("URL must start with mongo://")
  url <- sub("^mongodb://", "", url)
  if(!grepl("/", url))
    return(NULL)
  url <- sub("[^/]+/", "", url)
  url <- sub("[?].*$", "", url)
  url
}
