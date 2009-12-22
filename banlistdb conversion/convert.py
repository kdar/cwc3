import csv
import sys
from lxml import etree
from StringIO import StringIO

writer = csv.writer(sys.stdout, delimiter=",")

fp = open("localdb.xml", "r")
s = StringIO(fp.read())
tree = etree.parse(s)

items = tree.xpath("/database/scorelist/score")
dates = tree.xpath("/database/scorelist/score/lastseen/date")
times = tree.xpath("/database/scorelist/score/lastseen/time")

i = []

for x in range(len(items)):
  a = items[x].attrib
  timestr = dates[x].text + " " + times[x].text
  i.append((a['nick'].lower(), a['nick'], "", a['gamesplayed'], 0, 0, timestr, timestr))
  
writer.writerows(i)