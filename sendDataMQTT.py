#!/usr/bin/env python
import sys
import os
import paho.mqtt.publish as publish
import errno
from socket import error as socket_error

host = "148.202.23.200"
canal = "sensors/CICI/boards"
lecturas = {"temp":"temperature",
			"press":"pressure",
			"light":"light",
			"noise":"noise",
			"water":"water",
			"NO2":"NO2",
			"NH3":"NH3",
			"C3H8":"C3H8",
			"C4H10":"C4H10",
			"CH4":"CH4",
			"H2":"H2",
			"C2H5OH":"C2H5OH",
			"hum":"humidity"}
deli = ';'
if len(sys.argv)  == 2:
	tipoLectura = lecturas[sys.argv[1]]
	valueFile = open ('/media/mmcblk0p1/values.txt')
	valor = valueFile.readline()
	tipoLectura = tipoLectura + deli + str(valor)
	f = open('/sys/class/net/eth0/address','r')
	mac = f.readline()
	mensaje = tipoLectura + deli + "macAddress" + deli + mac.strip('\n')
	print mensaje
	f.close()
	valueFile.close()
	os.remove('/media/mmcblk0p1/values.txt')
	try:
		publish.single(canal, mensaje, hostname=host)
	except socket_error as serr:
		print "error :)"
