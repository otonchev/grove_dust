#!/usr/bin/env python

#
# This is a simple CGI script which shows latest Air Quality readings obtained from
# ../test_mysql
#
# Install the script and access it through a web browser:
#
#     http://192.168.0.240/cgi-bin/airquality.py
#

import MySQLdb
import sys

# parsing form data
import cgi
import cgitb
cgitb.enable()

db_user="root"
db_pass="pass"
db_name="AirQuality"

def cgi_error(msg):
    print "Content-type: text/html\n\n"
    print msg
    exit(0)

#connect to database, note that database Temperature must exist
try:
    conn = MySQLdb.connect(host="localhost", user=db_user, passwd=db_pass,
        db=db_name)
except MySQLdb.Error:
    cgi_error("unable to connect to database, wrong credentials or database"
        " %s does not exist?" % db_name)

cursor = conn.cursor()

#query air quality data, note that table ParticlePM25 must exist and there
#should be 2 fields: ts_created TIMESTAMP and aqi INT
#Only last 1 week is taken into account.
try:
    cursor.execute('SELECT ts_created, aqi FROM ParticlePM25 WHERE ts_created ='
        ' (SELECT MAX(ts_created) FROM ParticlePM25)');
except MySQLdb.Error:
    cgi_error("unable to query database, table ParticlePM25 does not exist?")

rows = cursor.fetchall()
if len(rows) != 1:
    cgi_error("no air quality data")

print "Content-type: text/html\n\n"
for (current_time, current_reading) in rows:
    print "AQI (Air Quality index) %d, obtained at: %s" % (current_reading, current_time)
