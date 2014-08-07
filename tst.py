#!/usr/bin/python
#-*- encoding:utf-8 -*-



import time, sys


for line, cnt in zip (sys.argv, xrange (len (sys.argv))):
	print ("Cmdline param: {0} - {1}".format (cnt, line))
print ("\n")

while True:
	print ("======= a simple string from pyt-script =======")
	time.sleep (5)
	
