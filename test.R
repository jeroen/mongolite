col <- mongo('mtcars',url = "mongodb+srv://readwrite:test@cluster0-84vdt.mongodb.net/test")
col$insert(mtcars)
col$count()
col$drop()

fs <- gridfs(url = "mongodb+srv://readwrite:test@cluster0-84vdt.mongodb.net/test")
fs$upload(R.home('doc/NEWS.pdf'))
fs$download('NEWS.pdf', 'news2.pdf')
tools::md5sum(c(R.home('doc/NEWS.pdf'), 'news2.pdf'))
buf <- fs$read('NEWS.pdf')
identical(buf, readBin(R.home('doc/NEWS.pdf'), raw(), 1e6))

fs$write('diamonds', serialize(ggplot2::diamonds, NULL))
unserialize(fs$read('diamonds'))


files <- mongo(col = 'fs.files', url = "mongodb+srv://readwrite:test@cluster0-84vdt.mongodb.net/test")
chunks <- mongo(col = 'fs.chunks', url = "mongodb+srv://readwrite:test@cluster0-84vdt.mongodb.net/test")


fs$drop()
