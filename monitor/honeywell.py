#!/usr/bin/python

# By Brad Goodman
# http://www.bradgoodman.com/
# brad@bradgoodman.com

####################### Fill in settings below #######################

USERNAME="derek.t.walton@gmail.com"
PASSWORD="&k&b#s&2NSZM7PY"
DEVICE_ID_1ST=9494612   # number at the end of URL of the honeywell control page
DEVICE_ID_2ND=3535964   # number at the end of URL of the honeywell control page

############################ End settings ############################

import urllib2
import urllib
import json
import datetime
import re
import time
import math
import base64
import time
import httplib
import sys
import getopt
import os
import stat
import subprocess
import string
    
AUTH="https://mytotalconnectcomfort.com/portal"

cookiere=re.compile('\s*([^=]+)\s*=\s*([^;]*)\s*')

def client_cookies(cookiestr,container):
  if not container: container={}
  toks=re.split(';|,',cookiestr)
  for t in toks:
    k=None
    v=None
    m=cookiere.search(t)
    if m:
      k=m.group(1)
      v=m.group(2)
      if (k in ['path','Path','HttpOnly']):
        k=None
        v=None
    if k: 
      #print k,v
      container[k]=v
  return container

def export_cookiejar(jar):
  s=""
  for x in jar:
    s+='%s=%s;' % (x,jar[x])
  return s

def logout(headers):
     # https://mytotalconnectcomfort.com/portal/Account/LogOff
     conn = httplib.HTTPSConnection("mytotalconnectcomfort.com");
     conn.request("GET", "https://mytotalconnectcomfort.com/portal/Account/LogOff",None,headers)
     r5 = conn.getresponse()
     print "logged out: " +  str(r5.status) + str(r5.reason)

def get_login():
    
    cookiejar=None
    headers={"Content-Type":"application/x-www-form-urlencoded",
            "Accept":"text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
            "Accept-Encoding":"sdch",
            "Host":"mytotalconnectcomfort.com",
            "DNT":"1",
            "Origin":"https://mytotalconnectcomfort.com/portal",
            "User-Agent":"Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/28.0.1500.95 Safari/537.36"
        }
    conn = httplib.HTTPSConnection("mytotalconnectcomfort.com")
    conn.request("GET", "/portal/",None,headers)
    r0 = conn.getresponse()
    #print r0.status, r0.reason
    
    for x in r0.getheaders():
      (n,v) = x
      #print "R0 HEADER",n,v
      if (n.lower() == "set-cookie"): 
        cookiejar=client_cookies(v,cookiejar)
    #cookiejar = r0.getheader("Set-Cookie")
    location = r0.getheader("Location")

    retries=5
    params=urllib.urlencode({"timeOffset":"240",
        "UserName":USERNAME,
        "Password":PASSWORD,
        "RememberMe":"false"})
    #print params
    newcookie=export_cookiejar(cookiejar)
    #print "Cookiejar now",newcookie
    headers={"Content-Type":"application/x-www-form-urlencoded",
            "Accept":"text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
            "Accept-Encoding":"sdch",
            "Host":"mytotalconnectcomfort.com",
            "DNT":"1",
            "Origin":"https://mytotalconnectcomfort.com/portal/",
            "Cookie":newcookie,
            "User-Agent":"Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/28.0.1500.95 Safari/537.36"
        }
    conn = httplib.HTTPSConnection("mytotalconnectcomfort.com")
    conn.request("POST", "/portal/",params,headers)
    r1 = conn.getresponse()
    #print r1.status, r1.reason
    
    for x in r1.getheaders():
      (n,v) = x
      #print "GOT2 HEADER",n,v
      if (n.lower() == "set-cookie"): 
        cookiejar=client_cookies(v,cookiejar)
    cookie=export_cookiejar(cookiejar)
    #print "Cookiejar now",cookie
    location = r1.getheader("Location")

    if ((location == None) or (r1.status != 302)):
        #raise BaseException("Login fail" )
        print("ErrorNever got redirect on initial login  status={0} {1}".format(r1.status,r1.reason))
        return


    #
    # Poll our devices
    #

    code=str(DEVICE_ID_1ST)

    t = datetime.datetime.now()
    utc_seconds = (time.mktime(t.timetuple()))
    utc_seconds = int(utc_seconds*1000)
    #print "Code ",code

    location="/portal/Device/CheckDataSession/"+code+"?_="+str(utc_seconds)
    #print "THIRD"
    headers={
            "Accept":"*/*",
            "DNT":"1",
            #"Accept-Encoding":"gzip,deflate,sdch",
            "Accept-Encoding":"plain",
            "Cache-Control":"max-age=0",
            "Accept-Language":"en-US,en,q=0.8",
            "Connection":"keep-alive",
            "Host":"mytotalconnectcomfort.com",
            "Referer":"https://mytotalconnectcomfort.com/portal/",
            "X-Requested-With":"XMLHttpRequest",
            "User-Agent":"Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/28.0.1500.95 Safari/537.36",
            "Cookie":cookie
        }
    conn = httplib.HTTPSConnection("mytotalconnectcomfort.com")
    #conn.set_debuglevel(999);
    #print "LOCATION R3 is",location
    conn.request("GET", location,None,headers)
    r3 = conn.getresponse()
    if (r3.status != 200):
      print("Error Didn't get 200 status on R3 status={0} {1}".format(r3.status,r3.reason))
      return


    # Print thermostat information returned

    #print r3.status, r3.reason
    rawdata=r3.read()
    j = json.loads(rawdata)
    #print "1st R3 Dump"
    #print json.dumps(j,indent=2)
    #print json.dumps(j,sort_keys=True,indent=4, separators=(',', ': '))
    #print "Success:",j['success']
    #print "Live",j['deviceLive']
    print "1st Indoor Temperature:",j['latestData']['uiData']["DispTemperature"]
    #print "Indoor Humidity:",j['latestData']['uiData']["IndoorHumidity"]
    #print "Cool Setpoint:",j['latestData']['uiData']["CoolSetpoint"]
    print "1st Heat Setpoint:",j['latestData']['uiData']["HeatSetpoint"]
    #print "Hold Until :",j['latestData']['uiData']["TemporaryHoldUntilTime"]
    #print "1st Status Cool:",j['latestData']['uiData']["StatusCool"]
    #print "1st Status Heat:",j['latestData']['uiData']["StatusHeat"]
    #print "1st Status Fan:",j['latestData']['fanData']["fanMode"]
    print "1st fanIsRunning:",j['latestData']['fanData']["fanIsRunning"]
    

    code=str(DEVICE_ID_2ND)

    t = datetime.datetime.now()
    utc_seconds = (time.mktime(t.timetuple()))
    utc_seconds = int(utc_seconds*1000)
    #print "Code ",code

    location="/portal/Device/CheckDataSession/"+code+"?_="+str(utc_seconds)
    #print "THIRD"
    headers={
            "Accept":"*/*",
            "DNT":"1",
            #"Accept-Encoding":"gzip,deflate,sdch",
            "Accept-Encoding":"plain",
            "Cache-Control":"max-age=0",
            "Accept-Language":"en-US,en,q=0.8",
            "Connection":"keep-alive",
            "Host":"mytotalconnectcomfort.com",
            "Referer":"https://mytotalconnectcomfort.com/portal/",
            "X-Requested-With":"XMLHttpRequest",
            "User-Agent":"Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/28.0.1500.95 Safari/537.36",
            "Cookie":cookie
        }
    conn = httplib.HTTPSConnection("mytotalconnectcomfort.com")
    #conn.set_debuglevel(999);
    #print "LOCATION R3 is",location
    conn.request("GET", location,None,headers)
    r3 = conn.getresponse()
    if (r3.status != 200):
      print("Error Didn't get 200 status on R3 status={0} {1}".format(r3.status,r3.reason))
      return


    # Print thermostat information returned

    #print r3.status, r3.reason
    rawdata=r3.read()
    j = json.loads(rawdata)
    #print "2nd R3 Dump"
    #print json.dumps(j,indent=2)
    #print json.dumps(j,sort_keys=True,indent=4, separators=(',', ': '))
    #print "Success:",j['success']
    #print "Live",j['deviceLive']
    print "2nd Indoor Temperature:",j['latestData']['uiData']["DispTemperature"]
    #print "Indoor Humidity:",j['latestData']['uiData']["IndoorHumidity"]
    #print "Cool Setpoint:",j['latestData']['uiData']["CoolSetpoint"]
    print "2nd Heat Setpoint:",j['latestData']['uiData']["HeatSetpoint"]
    #print "Hold Until :",j['latestData']['uiData']["TemporaryHoldUntilTime"]
    #print "2nd Status Cool:",j['latestData']['uiData']["StatusCool"]
    #print "2nd Status Heat:",j['latestData']['uiData']["StatusHeat"]
    #print "2nd Status Fan:",j['latestData']['fanData']["fanMode"]
    print "2nd fanIsRunning:",j['latestData']['fanData']["fanIsRunning"]
    
    logout(headers)

    return


def main():

  get_login()
  sys.exit()

if __name__ == "__main__":
    main()
