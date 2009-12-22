import md5
import pycurl
import urllib
import StringIO

#/banlist_download.php?list=others&sid=(PHPSESSIONID)

#---------------------------------
class Options:  
  class Banlistnl:
    login = "crenix"
    password = "yYXcOIf7"
    clientversion = "%33.%31+beta"
    host = "v2.banlist.nl"
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
      return "&".join(map(lambda x: "%s=%s" % (x, Options.Banlistnl.login_params[x]), Options.Banlistnl.login_params))

    #=================================
    @classmethod
    def GetDownloadParams(cls):
      return "&".join(map(lambda x: "%s=%s" % (x, Options.Banlistnl.download_params[x]), Options.Banlistnl.download_params))
    
data = StringIO.StringIO()

try:
  c = pycurl.Curl()
  c.setopt(c.URL, 'http://%s/%s?%s' % (Options.Banlistnl.host, Options.Banlistnl.login_script, Options.Banlistnl.GetLoginParams()))
  c.setopt(c.ENCODING, "gzip, deflate, identity")
  c.setopt(c.TIMEOUT, 3)
  c.setopt(c.WRITEFUNCTION, data.write)
  c.setopt(c.USERAGENT, "Mozilla/3.0 (compatible; Indy Library)")  
  c.perform()
  
  Options.Banlistnl.download_params['sid'] = data.getvalue().strip()

  data.seek(0)
  
  c.setopt(c.URL, 'http://%s/%s?%s' % (Options.Banlistnl.host, Options.Banlistnl.download_script, Options.Banlistnl.GetDownloadParams()))
  c.setopt(c.COOKIE, 'PHPSESSID=%s' % (Options.Banlistnl.download_params['sid']))
  c.setopt(c.HTTPHEADER, ["Connection: keep-alive", "Content-Type: multipart/form-data"])
  c.setopt(c.TIMEOUT, 200)
  c.perform()
  
  c.close()
  
  print data.getvalue()
except pycurl.error, e:
  print("pycurl: " + e[1])