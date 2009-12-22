#!/usr/bin/python

#ban database updater

from pysqlite2 import dbapi2 as sqlite
import sqlalchemy as sa
import pycurl
import re
import sys
import time

###############
#Options
###############

#---------------------------------
class Options(object):
  dburl = "sqlite:///cwc3.db"

###############
#Models
###############

#---------------------------------
class Banlist(object):
  #=================================
  def __init__(self, name, url):
    self.name = name
    self.url = url
  
  #=================================
  def __repr__(self):
    return "%s(%r, %r)" % (self.__class__.__name__, self.name, self.url)    

#---------------------------------
class BanBase(object):
  #=================================
  def __init__(self, name, reason):
    self.name = name
    self.reason = reason
  
  #=================================
  def __repr__(self):
    return "%s(%r, %r)" % (self.__class__.__name__, self.name, self.reason)  

###############
#Updater
###############

#---------------------------------
class Updater(object):
  #=================================
  def __init__(self):
    self.metadata = sa.BoundMetaData('sqlite:///cwc3.db')
    self.session = sa.create_session()
    self.InitTables()

  #=================================
  def Query(self, classobj):
    return self.session.query(classobj)
  
  #=================================
  def AppendData(self, data):
    self.data = self.data + data

  #=================================
  def InitTables(self):
    """ Creates and initializes the database tables.\n
        It uses the sqlalchemy mapper to map tables to objects """
    
    banlist_table = sa.Table('banlist', self.metadata,
      sa.Column('name', sa.String(50), primary_key=True),
      sa.Column('url', sa.String, nullable=False));      
    sa.mapper(Banlist, banlist_table)
    
    for x in self.Query(Banlist).select():      
      ban_table = sa.Table(x.name, self.metadata,
        sa.Column('id', sa.Integer, autoincrement=True, primary_key=True),
        sa.Column('name', sa.String(50)),
        sa.Column('reason', sa.String, nullable=False));
      ban_table.create(checkfirst=True)
      classname = x.name.capitalize()
      #Dynamic class creation.
      globals()[classname] = type(str(classname), (BanBase,), {})
      #mapper() takes a class as its first argument, so we must eval
      #the string, so it returns a class.
      sa.mapper(eval(classname), ban_table)
      
    #t = Asg("caca", "w00t")
    #self.session.save(t)
    #self.session.flush()
  
  #=================================
  def Run(self):   
    for x in self.Query(Banlist).select():
      self.data = ""
      print("Retrieving data from [%s]" % (x.name))
      if x.url == 'SPECIAL':
        function = "self.Get%s()" % (x.name.capitalize())
        data = eval(function)
      else:
        data = self.GetBasic(x.url)
      print("Done".ljust(50))
      
      classobj = eval(x.name.capitalize())
        
      #Now that we have the data, delete the previous contents
      #of the table so we don't have to deal with updates
      sa.class_mapper(classobj).local_table.drop()
      sa.class_mapper(classobj).local_table.create()
      
      sys.stdout.write("Creating database objects...")
      sys.stdout.flush()
      self.InjectIntoDb(classobj, data)
      print("DONE")
      sys.stdout.write("Flushing to disk...")
      sys.stdout.flush()
      self.session.flush()
      print("DONE")
  
  #=================================
  def Progress(self, download_t, download_d, upload_t, upload_d):
    if download_t == 0:
      sys.stdout.write("%d bytes retrieved\r" % (download_d))      
    else:
      percentage = download_d / download_t
      sys.stdout.write("%d/%d [%d%%]\r" % (download_d, download_t, percentage))
    sys.stdout.flush()
    
  #=================================
  def GetBasic(self, url):
    c = pycurl.Curl()
    c.setopt(c.URL, str(url))
    c.setopt(c.WRITEFUNCTION, self.AppendData)
    c.setopt(c.FOLLOWLOCATION, 1)
    c.setopt(c.MAXREDIRS, 5)
    c.setopt(c.NOPROGRESS, 0)
    c.setopt(c.PROGRESSFUNCTION, self.Progress)
    c.perform()
    c.close()
    
    return self.data
  
  #=================================
  def GetBanlistnl(self):
    print("GetBanlistnl")
    return self.data
  
  #=================================
  def InjectIntoDb(self, classobj, data):    
    rows = str(data).split("\n")
    p = re.compile('(.*?)\s(.*)')
    query = self.Query(classobj)
    con = sqlite.connect("cwc3.db")
    cur = con.cursor()
    for row in rows:      
      m = p.match(row.strip())
      if m:
        name = m.group(1)
        reason = m.group(2)
        cur.execute("insert into %s values (NULL, '%s', '%s')" % (classobj.__name__.lower(), name, reason.replace("'", "").replace('"', "")))
        #c = classobj(name, reason)
        #self.session.save(c)
    con.commit()

###############
#Main
###############      

#=================================
def main():
  updater = Updater()
  updater.Run()
  
#=================================
if (__name__ == "__main__"):
  main()