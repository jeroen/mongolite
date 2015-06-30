# Data column descriptions: https://www.kaggle.com/c/yelp-recsys-2013/data
# Total download size about 80MB (compressed)

# Required packages
library(mongolite)
library(curl)

# Collection with yelp 'users'
user <- mongo("user", db = "yelp")
user$import(gzcon(curl("https://jeroenooms.github.io/data/yelp_training_set_user.json.gz")))
user$count()
str(sample <- user$find(limit=10))
View(sample)

# Collection with yelp 'businesses'
business <- mongo("business", db = "yelp")
business$import(gzcon(curl("https://jeroenooms.github.io/data/yelp_training_set_business.json.gz")))
business$count()
str(sample <- business$find(limit=10))
View(sample)

# Collection with 'checkins'
# NB: weird (non-tidy) format: See https://www.kaggle.com/c/yelp-recsys-2013/data
checkin <- mongo("checkin", db = "yelp")
checkin$import(gzcon(curl("https://jeroenooms.github.io/data/yelp_training_set_checkin.json.gz")))
checkin$count()
str(sample <- checkin$find(limit=10))

# Collection with 'reviews' (80MB download)
review <- mongo("review", db = "yelp")
review$import(gzcon(curl("https://jeroenooms.github.io/data/yelp_training_set_review.json.gz")))
review$count()
str(sample <- review$find(limit=10))
View(sample)

# Make a plot
# devtools::install_github("rstudio/leaflet")
library(leaflet)
bd <- business$find('{"city" : "Phoenix", "review_count" :{"$gt" : 100}}')
m <- leaflet() %>% addTiles() %>%  addMarkers(lng=bd$longitude, lat=bd$latitude, popup=bd$name)
print(m)

# Phoenix neighborhood polygons: https://github.com/blackmad/neighborhoods/blob/master/phoenix.geojson
library(jsonlite)
shapes <- fromJSON('https://raw.githubusercontent.com/blackmad/neighborhoods/master/phoenix.geojson')
phoenix <- mongo("phoenix")
phoenix$insert(shapes$features)

# Get one shape
maryvale <- phoenix$find('{"properties.name" : "Maryvale"}')$geometry

# Total counts for business type:
counts <- business$mapreduce(
  map = "function(){this.categories.forEach(function(x){emit(x, 1)})}",
  reduce = "function(id, counts){return Array.sum(counts);}"
)

# Wordcloud for review text:
mapper <- "function(){
  var text = this.text.toLowerCase().replace(/[^a-z ]/g, '');
  var tags = text.split(' ');
  tags.forEach(function(x){
    if(x.length > 3) emit(x, 1);
  });
}"

reducer <- "function(id, counts){
  return Array.sum(counts);
}"

wordcloud <- review$mapreduce(mapper, reducer, query = '{"votes.useful" : {"$gt":10}}')
skiplist <- readLines("inst/google-10000-english-usa.txt", n = 500)
stopwords <- na.omit(match(skiplist, wordcloud[["_id"]]))
wordcloud <- wordcloud[-stopwords,]
wordcloud <- wordcloud[order(wordcloud$value, decreasing = TRUE),]
head(wordcloud, 100)

