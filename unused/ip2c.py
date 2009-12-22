import pycurl
import zlib
import StringIO
import gzip
import sqlalchemy as sa
import csv

class Options:
  class Ip2c:
    dburl = "sqlite:///ip2c.db"
    dbtable = "ip2country"
    fileInGzip = "IpToCountry.csv"
    
#=================================
def _FastMultiInsert(engine, table, data):
  """ A super fast multiple insert function that circumvents sqlalchemy's slow
      bulk insert mechanism. table is the name of the table or an instance of
      the sqlalchemy Table class you want to insert data into, and data is a list
      of dictionaries with columns and values """
      
  if isinstance(table, sa.Table):
    table = table.name
  
  if data and len(data) > 0:
    #We get the keys of the first data item. This let's us know what columns
    #we're going to be inserting data into.
    keys = data[0].keys()
    #Construct a list of columns that corresponds to sql syntax.
    cols = ",".join(keys)
    #Construct a list of mappings that sqlalchemy understands. These are basically
    #variables representing values that will be inserted via the data variable.
    maps = ",".join(map(lambda x: ":%s" % (x), keys))
    conn = engine.connect()
    conn.execute("INSERT INTO %s (%s) VALUES (%s)" % (table, cols, maps), data)
    conn.close()

engine = sa.create_engine(Options.Ip2c.dburl)
metadata = sa.BoundMetaData(engine)

table = sa.Table(Options.Ip2c.dbtable, metadata,
  sa.Column('id', sa.Integer, autoincrement=True, primary_key=True),
  sa.Column('ipfrom', sa.Integer),
  sa.Column('ipto', sa.Integer),
  sa.Column('registry', sa.String(20)),
  sa.Column('assigned', sa.Date),
  sa.Column('ctry', sa.String(5)),
  sa.Column('cntry', sa.String(5)),
  sa.Column('country', sa.String(50)));  
table.create(checkfirst=True)

data = StringIO.StringIO()

print("Downloading ip2country gzip file...")
try:
  c = pycurl.Curl()
  c.setopt(c.URL, 'http://software77.net/cgi-bin/ip-country/geo-ip.pl?action=download')
  #c.setopt(c.VERBOSE, 1)
  c.setopt(c.ENCODING, "gzip, deflate, identity")
  c.setopt(c.TIMEOUT, 200)
  c.setopt(c.WRITEFUNCTION, data.write)
  c.perform()
  c.close()
except pycurl.error, e:
  print("pycurl: " + e[1])

print("DONE")

#Must seek back the beginning so we can ungzip it.
data.seek(0)

print("Ungziping file...")
gzipper = gzip.GzipFile(filename=Options.Ip2c.fileInGzip, fileobj=data)
data = StringIO.StringIO(gzipper.read())

reader = csv.DictReader(data, fieldnames=['ipfrom','ipto','registry','assigned','ctry','cntry','country'])

pdata = []
for x in reader:
  pdata.append(x)
print("DONE")

table.delete().execute()  

print("Inserting into database...")
_FastMultiInsert(engine, table, pdata)
print("DONE")
