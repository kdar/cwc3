#!/usr/bin/python

#Note: When using sqlite, don't iterate on table.select().execute().
#First call table.select().execute() and save it to a variable say e,
#then call e.fetchall() and iterate on that. If you don't, you will get
#table locking errors.

#Performance: I've had a lot of trouble with performance with sqlalchemy and
#it took me a while to find a solution. First off, I tried using the ORM and
#sessions, and adding all the new objects to the session and flushing it, and that
#was HORRIBLE performance wise. I also tried ORM with just inserting each object
#by itself to the database and that was still bad at performance. So I ditched the
#ORM and decided to use just the abstraction layer of sqlalchemy, and tried inserting
#a row at a time. This was bad performance as it would commit after every insert. So
#then I made a transaction in sqlalchemy, so it would do a bunch of inserts, then
#commit the transaction. The performance of this was MUCH better, but still not as
#good as it could be. The next idea was to use the executemany function of the DBAPI,
#which sqlalchemy supports. We would compose a list of dictionaries containing the
#data we want to insert, then call table.insert().execute(data). This was decent
#in performance, but still not as good as it could be. The reason being sqlalchemy
#has to construct the query itself, and it takes 2n operations to do this. The best
#solution I found was using the executemany function of the engine connection directly,
#and supplying my own insert statement. I know this isn't as cross-dbms as it should be,
#but just for inserting it's not bad. I used this statement:
#conn.execute("INSERT INTO %s VALUES (NULL, :name, :reason)" % (table.name), data)
#data being a list of dictionaries with name and reason mappings. This is the best in
#performance you can get.
#Advice: My advice is to use ORM for querying and everything else, but then make a separate
#function that can do inserts and bulk inserts, possibly taking an object as its parameter.
#The high level functionality is great for selects and stuff, especially since you can
#compile queries. But inserting a load of data is horrible. I'm not sure how diverse
#all the dbms's are at the INSERT sql command, but you could easily accomodate for different
#dbms's in the function.

import sqlalchemy as sa
import pycurl
import re
import sys
import time
import zlib
import StringIO
import gzip
import csv
import md5
from optparse import OptionParser

###############
#Options
###############

#---------------------------------
class Options(object):
  #---------------------------------
  class BasicBan:
    dburl = "sqlite:///local.db"
  
  #---------------------------------
  class Ip2c:
    dburl = "sqlite:///local.db"
    dbtable = "ip2country"
    gzipUrl = "http://software77.net/cgi-bin/ip-country/geo-ip.pl?action=download"
    fileInGzip = "IpToCountry.csv"
  
  #---------------------------------
  class Banlist:
    dburl = "sqlite:///local.db"
    login = "crenix"
    password = "yYXcOIf7"
    clientversion = "%33.%31+beta"
    host = "banlist.nl"
    login_script = "banlist_login.php"
    download_script = "banlist_download.php"
    login_params = {'login': login,
      'pass': md5.new(password).hexdigest(),
      'clientversion': clientversion} #urllib.quote(clientversion)}
    download_params = {'list': "others",
      'sid': 0}
    
    #=================================
    @classmethod
    def GetLoginParams(cls):
      return "&".join(map(lambda x: "%s=%s" % (x, Options.Banlist.login_params[x]), Options.Banlist.login_params))

    #=================================
    @classmethod
    def GetDownloadParams(cls):
      return "&".join(map(lambda x: "%s=%s" % (x, Options.Banlist.download_params[x]), Options.Banlist.download_params))
    
    
###############
#Base updater class
###############

#---------------------------------
class UpdaterBase(object):
  #=================================
  def _FastMultiInsert(self, engine, table, data):
    """ A super fast multiple insert function that circumvents sqlalchemy's slow
        bulk insert mechanism. 
        
        Params:
        engine - the sqlalchemy engine of the database we are inserting into
        table - the name of the table or an instance of the sqlalchemy Table class you want to insert data into
        data - a list of dictionaries with columns and values """
        
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
      conn.execute(u"INSERT INTO %s (%s) VALUES (%s)" % (table, cols, maps), data)
      conn.close()
      
  #=================================
  def _Progress(self, download_t, download_d, upload_t, upload_d):
    """ Used by pycurl to display progress """
    
    if download_t == 0:
      sys.stdout.write("  %d bytes retrieved\r" % (download_d))      
    else:
      percentage = (download_d / download_t) * 100
      sys.stdout.write("  %d/%d [%d%%]\r" % (download_d, download_t, percentage))
    sys.stdout.flush()

###############
#BasicBanUpdater
###############

#---------------------------------
class BasicBanUpdater(UpdaterBase):
  """ A class that updates the ban databases """
  
  #=================================
  def __init__(self):
    self.engine = sa.create_engine(Options.BasicBan.dburl)
    self.metadata = sa.MetaData(self.engine)
    self._InitTables()
  
  #=================================
  def _InitTables(self):
    """ Creates and initializes the basic ban database tables. """
    
    self.config_table = sa.Table('config', self.metadata,
      sa.Column('name', sa.String(50), primary_key=True),
      sa.Column('url', sa.String, nullable=False),
      sa.Column('type', sa.String(20), nullable=False));
    #fetchall releases any locks
    results = self.config_table.select().execute().fetchall()
    for x in results:
      #Only process this entry if it is basic
      if x['type'] == "basic":
        #Adds the table as a member of this object. The name of the variable
        #is the name of the table, with _table appended to it.      
        table = sa.Table(x['name'], self.metadata,
          sa.Column('id', sa.Integer, autoincrement=True, primary_key=True),
          sa.Column('lname', sa.String(50), index=True),
          sa.Column('name', sa.String(50)),
          sa.Column('reason', sa.String, nullable=False));
        table.create(checkfirst=True)
        setattr(self, "%s_table" % (x['name']), table)
  
  #=================================
  def Run(self):
    """ This function runs the updater. """
    
    print("BasicBanUpdater:");
    
    results = self.config_table.select().execute().fetchall()
    for x in results:
      self.data = ""
      if x['type'] == "basic"  and x['name'] == 'tda':
        print("  Retrieving data from [%s]" % (x['name']))      
        data = self._GetBasic(x['url'])
                
        if data:          
          print("  Done".ljust(50))
        else:
          print("Failed to get bans.".ljust(50))
          continue
      
        table = getattr(self, "%s_table" % (x['name']))
          
        #Now that we have the data, delete the previous contents
        #of the table so we don't have to deal with updates
        table.delete().execute()
        
        sys.stdout.write("  Inserting into database...")
        sys.stdout.flush()
        self._InjectIntoTable(table, data)
        print("  DONE")
 
  #=================================
  def _GetBasic(self, url):
    """ Gets a basic url. All it will do is access the url and download the contents """
    
    data = StringIO.StringIO()
    
    try:
      c = pycurl.Curl()
      c.setopt(c.URL, str(url))
      c.setopt(c.WRITEFUNCTION, data.write)
      c.setopt(c.FOLLOWLOCATION, 1)
      c.setopt(c.MAXREDIRS, 5)
      c.setopt(c.NOPROGRESS, 0)
      c.setopt(c.PROGRESSFUNCTION, self._Progress)
      c.perform()
      c.close()
    except pycurl.error, e:
      print("  pycurl: " + e[1])
      return False
  
    return data.getvalue()

  #=================================
  def _InjectIntoTable(self, table, data):
    """ This function takes a Table object and data that is newline delimited, and
        will inject into this Table a name and reason that is extracted from each line """
    
    rows = str(data).split("\n")
    p = re.compile('(.*?)\s(.*)')
    #We're done with parsing data, so reuse it.
    data = []
    for row in rows:      
      m = p.match(row.strip())
      if m:
        try:
          data.append({u'lname': unicode(m.group(1).lower()), u'name': unicode(m.group(1)), u'reason': unicode(m.group(2))})
        except:
          pass
    #Decent performance, but about 2 times slower than the next statement
    #i = table.insert().execute(data)
    #As fast as you can get.
    self._FastMultiInsert(self.engine, table, data)

###############
#BanlistUpdater
###############

#---------------------------------
class BanlistUpdater(UpdaterBase):
  #=================================
  def __init__(self):
    self.engine = sa.create_engine(Options.Banlist.dburl)
    self.metadata = sa.MetaData(self.engine)
    self._InitTables()
  
  #=================================
  def _InitTables(self):
    """ Creates and initializes the basic ban database tables. """
    
    self.config_table = sa.Table('config', self.metadata,
      sa.Column('name', sa.String(50), primary_key=True),
      sa.Column('url', sa.String, nullable=False),
      sa.Column('type', sa.String(20), nullable=False));
    #fetchall releases any locks
    results = self.config_table.select().execute().fetchall()
    for x in results:
      #Only process this entry if it is basic
      if x['type'] == "banlist":
        #Adds the table as a member of this object. The name of the variable
        #is the name of the table, with _table appended to it.      
        table = sa.Table(x['name'], self.metadata,
          sa.Column('id', sa.Integer, autoincrement=True, primary_key=True),
          sa.Column('lname', sa.String(50), index=True),
          sa.Column('name', sa.String(50)),
          sa.Column('reason', sa.String, nullable=False),
          sa.Column('source', sa.String(50)),
          sa.Column('deleted', sa.Integer));
        table.create(checkfirst=True)
        setattr(self, "%s_table" % (x['name']), table)
  
  #=================================
  def Run(self):
    """ This function runs the updater. """
    
    print("BanlistUpdater:");
    
    results = self.config_table.select().execute().fetchall()
    for x in results:
      self.data = ""
      if x['type'] == "banlist":
        print("  Retrieving data from [%s]" % (x['name']))      
        data = self._GetBanlist()
                
        if data:          
          print("  Done".ljust(50))
        else:
          print("Failed to get bans.".ljust(50))
          continue
      
        table = getattr(self, "%s_table" % (x['name']))
          
        #Now that we have the data, delete the previous contents
        #of the table so we don't have to deal with updates
        table.delete().execute()
        
        sys.stdout.write("  Inserting into database...")
        sys.stdout.flush()
        self._InjectIntoTable(table, data)
        print("  DONE")

   #=================================
  def _GetBanlist(self):
    """ Get the bans from banlist """
    
    data = StringIO.StringIO()

    try:
      c = pycurl.Curl()
      c.setopt(c.URL, 'http://%s/%s?%s' % (Options.Banlist.host, Options.Banlist.login_script, Options.Banlist.GetLoginParams()))
      c.setopt(c.ENCODING, "gzip, deflate, identity")
      c.setopt(c.TIMEOUT, 3)
      c.setopt(c.WRITEFUNCTION, data.write)
      c.setopt(c.PROGRESSFUNCTION, self._Progress)
      c.setopt(c.USERAGENT, "Mozilla/3.0 (compatible; Indy Library)")
      c.perform()
      
      Options.Banlist.download_params['sid'] = data.getvalue().strip()
    
      data.seek(0)
      
      c.setopt(c.URL, 'http://%s/%s?%s' % (Options.Banlist.host, Options.Banlist.download_script, Options.Banlist.GetDownloadParams()))
      c.setopt(c.COOKIE, 'PHPSESSID=%s' % (Options.Banlist.download_params['sid']))
      c.setopt(c.HTTPHEADER, ["Connection: keep-alive", "Content-Type: multipart/form-data"])
      c.setopt(c.TIMEOUT, 200)
      c.perform()
      
      c.close()
    except pycurl.error, e:
      print("pycurl: " + e[1])
      return False
      
    return data.getvalue()

  #=================================
  def _InjectIntoTable(self, table, data):
    """ This function takes a Table object and data that is newline delimited, and
        will inject into this Table name, reason, source, and deleted that is extracted from each line """
    
    rows = str(data).split("\n")
    p = re.compile('(.*?)\t(.*)\t(.*)\t(.*)')
    #We're done with parsing data, so reuse it.
    data = []
    for row in rows:      
      m = p.match(row.strip())
      if m:
        data.append({'lname': m.group(1).lower(), 'name': m.group(1), 'reason': m.group(2), 'source': m.group(3), 'deleted': m.group(4)})
    self._FastMultiInsert(self.engine, table, data)


###############
#Ip2cUpdater
###############

#---------------------------------
class Ip2cUpdater(UpdaterBase):
  #=================================
  def __init__(self):
    self.engine = sa.create_engine(Options.Ip2c.dburl)
    self.metadata = sa.MetaData(self.engine)
    self._InitTables()
    
  #=================================
  def _InitTables(self):
    """ Creates and initializes the database tables. """
    
    self.table = sa.Table(Options.Ip2c.dbtable, self.metadata,
      sa.Column('id', sa.Integer, autoincrement=True, primary_key=True),
      sa.Column('ipfrom', sa.Integer, index=True),
      sa.Column('ipto', sa.Integer, index=True),
      sa.Column('registry', sa.String(20)),
      sa.Column('assigned', sa.Date),
      sa.Column('ctry', sa.String(5)),
      sa.Column('cntry', sa.String(5)),
      sa.Column('country', sa.String(50)));  
    self.table.create(checkfirst=True)
    
  #=================================
  def Run(self):
    """ This function runs the updater. """
    
    print("Ip2cUpdater:");
    
    data = StringIO.StringIO()

    print("  Downloading ip2country gzip file...")
    try:
      c = pycurl.Curl()
      c.setopt(c.URL, Options.Ip2c.gzipUrl)
      #c.setopt(c.VERBOSE, 1)
      c.setopt(c.ENCODING, "gzip, deflate, identity")
      c.setopt(c.TIMEOUT, 200)
      c.setopt(c.WRITEFUNCTION, data.write)
      c.setopt(c.NOPROGRESS, 0)
      c.setopt(c.PROGRESSFUNCTION, self._Progress)
      c.perform()
      c.close()
    except pycurl.error, e:
      print("  pycurl: " + e[1])
      return False
    
    print("  Done".ljust(50))
    
    #Must seek back the beginning so we can ungzip it.
    data.seek(0)
    
    sys.stdout.write("  Ungziping file...")
    sys.stdout.flush()
    
    #Ungzip the file received.
    gzipper = gzip.GzipFile(filename=Options.Ip2c.fileInGzip, fileobj=data)
    data = StringIO.StringIO(gzipper.read())
    
    #Use csv to read the file. Makes filtering comments easier.
    reader = csv.DictReader(data, fieldnames=['ipfrom','ipto','registry','assigned','ctry','cntry','country'])
    
    #We must put each dictionary in a list so FastMultiInsert can use it.
    #We either do this, or change FastMultiInsert to accomodate gzip reader objects.
    pdata = []
    for x in reader:
      pdata.append(x)
    print("  DONE")
    
    #Delete the previous contents of the table.
    self.table.delete().execute()  
    
    sys.stdout.write("  Inserting into database...")
    sys.stdout.flush()
    self._FastMultiInsert(self.engine, self.table, pdata)
    print("  DONE")

###############
#Main
###############

#=================================
def Main():
  parser = OptionParser()
  parser.add_option("-b", "--basic_ban", 
                    action="store_true", dest="basic_ban", default=False,
                    help="Update the basic ban database")
  parser.add_option("-l", "--banlist",
                    action="store_true", dest="banlist", default=False,
		                help="Update the banlist ban database")
  parser.add_option("-i", "--ip2c",
                    action="store_true", dest="ip2c", default=False,
                    help="Update the ip2c database")
  
  (options, args) = parser.parse_args()
  
  #If no options are supplied, do all
  if not options.basic_ban and not options.banlist and not options.ip2c:
    options.basic_ban = options.banlist = options.ip2c = True
  
  try:
    if options.basic_ban:
      basicBanUpdater = BasicBanUpdater()
      basicBanUpdater.Run()
    
    if options.banlist:
      banlist = BanlistUpdater()
      banlist.Run()
      
    if options.ip2c:
      ip2cUpdater = Ip2cUpdater()
      ip2cUpdater.Run()
  except KeyboardInterrupt:
    print("")
    sys.exit(-1)
  
#=================================
if (__name__ == "__main__"):
  Main()
  
