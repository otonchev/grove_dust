#!/usr/bin/env python

#
# This is a simple CGI script which plots Air Quality readings obtained from
# ../test_mysql using the matplotlib library.
#
# Install the script and access it through a web browser:
#
#     http://192.168.0.240/cgi-bin/plot_airquality.py
#

import MySQLdb
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as pyplot
import matplotlib.dates as md
import cStringIO
import sys

# parsing form data
import cgi
import cgitb
cgitb.enable()

db_user="root"
db_pass="pass"
db_name="AirQuality"
db_table="ParticlePM25"

def cgi_error(msg):
    print "Content-type: text/html\n\n"
    print msg
    exit(0)

#connect to the database
try:
    conn = MySQLdb.connect(host="localhost", user=db_user, passwd=db_pass,
        db=db_name)
except MySQLdb.Error:
    cgi_error("unable to connect to database, wrong credentials or database"
        " %s does not exist?" % db_name)

cursor = conn.cursor()

#query air quality data, only readings from the last 2 days are of interest
try:
    cursor.execute('SELECT ts_created, aqi FROM %s WHERE ts_created >= '
        'DATE_ADD(CURDATE(), INTERVAL -1 DAY)' % db_table);
except MySQLdb.Error:
    cgi_error("unable to query database, table %s does not exist?" % db_table)

rows = cursor.fetchall()
if len(rows) == 0:
    cgi_error("no air quality data")

#create two lists, one containing air quality readings, the other one
#timestamps
air_quality_data = []
dates = []

for (current_time, current_reading) in rows:
    air_quality_data = air_quality_data + [current_reading]
    dates = dates + [current_time]

#plot data into a .svg image
format = "svg"
sio = cStringIO.StringIO()
fig = pyplot.figure()
fig.suptitle('AQI (Air Quality index)', fontsize=20)
pyplot.subplots_adjust(bottom=0.2)
pyplot.xticks(rotation=25)

ax=pyplot.gca()
xfmt = md.DateFormatter('%Y-%m-%d %H:%M:%S')
ax.xaxis.set_major_formatter(xfmt)

pyplot.plot(dates, air_quality_data)
pyplot.savefig(sio, format=format)

#send image data to the web server
sys.stdout.write("Content-Type: text/xml\r\n\r\n")
sys.stdout.write(sio.getvalue())
