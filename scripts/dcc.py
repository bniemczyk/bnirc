#!/usr/bin/env python

###########################################################################
#   Copyright (C) 2005 by Andrew Riedi                                    #
#   andrew@snprogramming.com                                              #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License version 2, as    #
#   published by the Free Software Foundation.                            #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
###########################################################################

import socket
import bnirc
import time
import string
import os
import struct
import user

class dcc_info:
	pass

dcc_info.people = []
dcc_info.files_who = []

class server:
	def __init__(self):
		self.dcc_ip = bnirc.StringOption("dcc_ip")
		if self.dcc_ip:
			self.ip = self.dcc_ip
		else:
        		self.ip = bnirc.CurrentIp()
		self.orig = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.orig.bind((self.ip, 0)) # pick a random (and open) port
		self.orig.listen(5)

		self.name = ""
		self.isclient = 0
		self.isconnected = 0
		self.size = 0
		self.file = ""
		self.out_file = ""
		self.dcc_type = ""
		self.i = 0
		self.data_sent = 0
		self.len = 0
		
		self.PORT = self.orig.getsockname()[1]
		bnirc.DebugMsg("DCC server started on port " + str(self.PORT) + ".")

		bnirc.RegisterPoll(self.orig.fileno(), self.dcc_accept)
		
		return None
	
	def dcc_accept(self):
		self.con, self.addr = self.orig.accept()
		# sleep needed, or self.con wont be there for RegisterPoll
		time.sleep(.5)
		if self.dcc_type == "chat":
			bnirc.RegisterPoll(self.con.fileno(), self.dcc_get_msg)
		bnirc.DebugMsg("DCC accepted with " + self.name + ".")
		self.isconnected = 1
		if self.dcc_type == "send":
			self.fp = open(self.file, "rb")
			bnirc.RegisterPoll(self.con.fileno(), self.dcc_send_file)
			self.dcc_send_file()
		
	def dcc_get_msg(self):
		try:
			data = self.con.recv(512)
			if data == "":
				self.dcc_close()
				return 0

			bnirc.IncomingMsg("DccFrom(%s)" % (self.name), data)
		except:
			return 1
		
	def dcc_send_msg(self, msg):
		bnirc.IncomingMsg("DccTo(%s)" % (self.name), msg)
		self.con.send(msg + "\n")

		return 1

	def dcc_send_file(self):
		data = self.fp.read(4194304)
		self.len += len(data)
		#self.data_sent = self.data_sent + len(data)

		#if self.data_sent == self.size:
		if data == "":
			self.dcc_close()
			return 0

		self.con.send(data)

		return 1

	def dcc_close(self):
		bnirc.UnregisterPoll(self.con.fileno())
		bnirc.UnregisterPoll(self.orig.fileno())
		self.orig.shutdown(2)
		time.sleep(.5)
		self.orig.close()
		self.con.shutdown(2)
		time.sleep(.5)
		self.con.close()

		if self.dcc_type == "chat":
			bnirc.DebugMsg("DCC closed with " + dcc_info.people[self.i].name + ".")
			del dcc_info.people[self.i]
		elif self.dcc_type == "send":
			bnirc.DebugMsg("DCC closed with " + dcc_info.files_who[self.i].name + ".")
			self.fp.close()
			bnirc.DebugMsg(self.file + " was sent successfully to " + self.name + ".")
			del dcc_info.files_who[self.i]

		return 1

class client:
	def __init__(self):
		self.name = ""
		self.isclient = 1

		self.ip = ""
		self.port = 0
		self.isconnected = 0
		self.size = 0
		self.file = ""
		self.out_file = ""
		self.dcc_type = ""
		self.i = 0
		self.data_sent = 0
		self.len = 0
		
		return None
	
	def dcc_connect(self):
		self.con = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.con.connect((ascii_to_ip(self.ip), int(self.port)))
		self.isconnected = 1
		if self.dcc_type == "chat":
			bnirc.RegisterPoll(self.con.fileno(), self.dcc_get_msg)
		elif self.dcc_type == "send":
			#self.con.blocking(1)
			# add an option to specify download dir
			#if self.out_file = "":
			#	self.out_file = self.file
			self.fp = open((user.home + "/.bnIRC/" + self.out_file), "wb")
			bnirc.RegisterPoll(self.con.fileno(), self.dcc_get_file)

		return 1
		
	def dcc_get_msg(self):
		try:
			data = self.con.recv(512)
			if data == "":
				self.dcc_close()
				return 0

			bnirc.IncomingMsg("DccFrom(%s)" % (self.name), data)
		except:
			return 1
	
	def dcc_get_file(self):
		try:
			data = self.con.recv(4194304)
			self.len += len(data)

			if data == "":
				self.dcc_close()
				return 0

			self.con.send(struct.pack('L', socket.htonl(self.len)))
			self.fp.write(data)
		except:
			return 1

	def dcc_send_msg(self, msg):
		bnirc.IncomingMsg("DccTo(%s)" % (self.name), msg)
		self.con.send(msg + "\n")

		return 1

	def dcc_close(self):
		# add UnregisterPoll for dcc_get_file and dcc_get_msg
		bnirc.UnregisterPoll(self.con.fileno())
		self.con.shutdown(2)
		time.sleep(.5)
		self.con.close()

		if self.dcc_type == "chat":
			bnirc.DebugMsg("DCC closed with " + dcc_info.people[self.i].name + ".")
			del dcc_info.people[self.i]
		elif self.dcc_type == "send":
			bnirc.DebugMsg(self.out_file + " was received successfully from " + self.name + ".")
			self.fp.close()
			bnirc.DebugMsg("DCC closed with " + dcc_info.files_who[self.i].name + ". [" + self.file + "]")
			del dcc_info.files_who[self.i]

		return 1

def dcc_chat(args):
        name = string.lower(args[1])
        type = "DCC CHAT "
        file = "chat"

	i = 0
	how_many = len(dcc_info.people)

	while i < how_many:
		if (dcc_info.people[i].name == name) and (dcc_info.people[i].isclient) and (dcc_info.people[i].isconnected == 0):
			dcc_info.people[i].dcc_connect()
			return 0

		elif dcc_info.people[i].name == name:
			bnirc.DebugMsg("Already connected to: " + dcc_info.people[i].name)
			return 0

		i = i + 1

	if name == string.lower(bnirc.GetCurrentInfo()[0]):
		bnirc.DebugMsg("Cant connect to self!")
		return 0

	x = server() # declare x here, so that we can have n instances of x
	x.name = name
	x.dcc_type = "chat"
	dcc_info.people.append(x) # make x availible outside this function
	x.i = (len(dcc_info.people) - 1)

	dcc_ip = bnirc.StringOption("dcc_ip")
	if dcc_ip:
		ip = ip_to_ascii(dcc_ip)
	else:
		ip = ip_to_ascii(bnirc.CurrentIp())
	
	ctcp_string = "PRIVMSG " + name + " :\1" + type + file + " " + ip + " " + str(x.PORT) + " \1\n"

	bnirc.IrcRaw(ctcp_string)

        return 0

def dcc_send(args):
        name = string.lower(args[1])
        type = "DCC SEND "
        file = args[2]

	dcc_ip = bnirc.StringOption("dcc_ip")
	if dcc_ip:
		ip = ip_to_ascii(dcc_ip)
	else:
        	ip = ip_to_ascii(bnirc.CurrentIp())
		
	x = server()
	x.name = name
	x.file = file
	x.out_file = file
	x.size = str(int(os.stat(file)[6]))
	x.dcc_type = "send"
	dcc_info.files_who.append(x) # make x availible outside this function
	x.i = (len(dcc_info.files_who) - 1)
	
        ctcp_string = "PRIVMSG " + name + " :\1" + type + file.split('/')[-1] + " " + ip + " " + str(x.PORT) + " " + x.size + " \1\n"

        bnirc.IrcRaw(ctcp_string)

        return 1

def dcc_get(args):
        name = string.lower(args[1])

	if len(args) == 4:
		dcc_info.files_who[0].out_file = args[3]
	else:
		dcc_info.files_who[0].out_file = args[2]

	i = 0
	how_many = len(dcc_info.files_who)

	while i < how_many:
		if (dcc_info.files_who[i].name == name) and (dcc_info.files_who[i].isclient) and (dcc_info.files_who[i].isconnected == 0):
			dcc_info.files_who[i].dcc_connect()
			return 0

		elif dcc_info.files_who[i].name == name:
			bnirc.DebugMsg("Already connected to: " + dcc_info.files_who[i].name)
			return 0

		i = i + 1

        return 1

def dcc_send_msg_wrapper(args):
	i = 0
	how_many = len(dcc_info.people)

	while i < how_many:
		if dcc_info.people[i].name == string.lower(args[1]):
			dcc_info.people[i].dcc_send_msg(args[2])
		i = i + 1
		
	return 1

def dcc_close_wrapper(args):
	i = 0
	how_many = len(dcc_info.people)
	
	while i < how_many:
		if dcc_info.people[i].name == args[1]:
			dcc_info.people[i].dcc_close()
		i = i + 1

	return 1

def ip_to_ascii(args): # rewrite with bitwise operations
        return str((int(args.split('.')[0]) * 16777216) + (int(args.split('.')[1]) * 65536) + (int(args.split('.')[2]) * 256) + int(args.split('.')[3]))

def ascii_to_ip(args):
	first = str(int(args) / 16777216)
	second = str((int(args) % 16777216) / 65536)
	third = str(((int(args) % 16777216) % 65536) / 256)
	forth = str(((int(args) % 16777216) % 65536) % 256)

	return first + "." + second + "." + third + "." + forth
	
def check_dcc(args):
        type = ""
        if args[3][1:4] == "DCC":
                if args[3][5:9] == "CHAT":
                	type = "chat"
			dcc_type = "chat"
                elif args[3][5:9] == "SEND":
			file_size = int(args[3][10:].split()[3].split('\1')[0])
			file_size_type = "bytes"
			if file_size > 1024:
				file_size = file_size / 1024
				file_size_type = "kilobytes"
			if file_size > 1024:
				file_size = file_size / 1024
				file_size_type = "megabytes"
			if file_size > 1024:
				file_size = file_size / 1024
				file_size_type = "gigabytes"
			
			file_size = str(file_size)

                        type = "send " + args[3][10:].split()[0] + " " + file_size + " " + file_size_type
			dcc_type = "send"

                bnirc.UserMsg("DCC request from " + string.lower(bnirc.GrabNick(args[0])) + " of type: " + type)
		if type == "chat":
			bnirc.UserMsg("Type: '" + bnirc.StringOption("command_char") + "dcc:chat <nick>' to accept.")
		elif type[:4] == "send":
			bnirc.UserMsg("Type: '" + bnirc.StringOption("command_char") + "dcc:get <nick> <file> [file_name]' to accept.")
	else:
		return 1

	x = client() # declare x here, so that we can have n instances of x
	x.name = string.lower(bnirc.GrabNick(args[0]))
	x.ip = args[3][10:].split()[1]
	x.port = args[3][10:].split()[2].split('\1')[0]
	x.dcc_type = dcc_type
	if dcc_type == "chat":
		dcc_info.people.append(x) # make x availible outside this function
		x.i = (len(dcc_info.people) - 1)
	elif dcc_type == "send":
		x.size = args[3][10:].split()[3].split('\1')[0]
		x.file = args[3][10:].split()[0].split('\1')[0]
		dcc_info.files_who.append(x) # make x availible outside this function
		x.i = (len(dcc_info.people) - 1)

        return 1

def dcc_status(args):
	dashes = ""
	for i in range(80 - 9):  # MAKE A PORT ALREADY OF SCREEN SIZE!!!!
		dashes += "-"

	i = 0
	how_many = len(dcc_info.people)

	bnirc.UserMsg("DCC:CHAT" + dashes)
	while i < how_many:
		bnirc.DebugMsg(dcc_info.people[i].name)
		i = i + 1
		
	i = 0
	how_many = len(dcc_info.files_who)

	bnirc.UserMsg("DCC:SEND" + dashes)
	while i < how_many:
		if len(str(float(dcc_info.files_who[i].len)/float(dcc_info.files_who[i].size))[2:4]) < 2:
			percent = str(float(dcc_info.files_who[i].len)/float(dcc_info.files_who[i].size))[2:4] + "0%)"
		else:
			percent = str(float(dcc_info.files_who[i].len)/float(dcc_info.files_who[i].size))[2:4] + "%)"

		if "dcc_send_file" in dir(dcc_info.files_who[i]):
			bnirc.DebugMsg(dcc_info.files_who[i].name + "\t" + dcc_info.files_who[i].out_file + "\t" + str(dcc_info.files_who[i].len) + "/" + str(dcc_info.files_who[i].size) + " (" + percent)
		i = i + 1

	i = 0
	bnirc.UserMsg("DCC:GET-" + dashes)
	while i < how_many:
		if len(str(float(dcc_info.files_who[i].len)/float(dcc_info.files_who[i].size))[2:4]) < 2:
			percent = str(float(dcc_info.files_who[i].len)/float(dcc_info.files_who[i].size))[2:4] + "0%)"
		else:
			percent = str(float(dcc_info.files_who[i].len)/float(dcc_info.files_who[i].size))[2:4] + "%)"

		if "dcc_get_file" in dir(dcc_info.files_who[i]):
			bnirc.DebugMsg(dcc_info.files_who[i].name + "\t" + dcc_info.files_who[i].out_file + "\t" + str(dcc_info.files_who[i].len) + "/" + str(dcc_info.files_who[i].size) + " (" + percent)
		i = i + 1

	return 0

bnirc.RegisterServerString("privmsg", 1, 4, check_dcc)

bnirc.RegisterCommand("dcc:chat", "dcc:chat <nick>", "Attempts to start a DCC chat session.", 2, 2, dcc_chat)
bnirc.RegisterCommand("dcc:send", "dcc:send <nick> <file>", "Attempts to send a file over DCC.", 3, 3, dcc_send)
bnirc.RegisterCommand("dcc", "dcc <nick> <messsage>", "Sends a message over DCC chat.", 3, 3, dcc_send_msg_wrapper)
bnirc.RegisterCommand("dcc:close", "dcc:close <nick>", "Closses a DCC session.", 2, 2, dcc_close_wrapper)
bnirc.RegisterCommand("dcc:get", "dcc:get <nick> <file> [file_name]", "Accepts a file to be sent over DCC.", 3, 4, dcc_get)
bnirc.RegisterCommand("dcc:status", "dcc:status", "Displays the status of all your DCC connections at that moment in time.", 1, 1, dcc_status)

bnirc.DebugMsg("dcc script loaded")

