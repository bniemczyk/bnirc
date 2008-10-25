############################################
# Py-TOC 2.4
#
# Jamie Turner <jamwt@jamwt.com>
#
# Edited for use by BnIRC by Brandon Niemczyk <brandon@snprogramming.com> 8/10/2005
#

_VERSION = "2.4"

import socket
import select
import re
import struct
import whrandom
import sys
import time

import bnirc

TOC_SERV_AUTH = ("login.oscar.aol.com", 5159 )
TOC_SERV = ( "toc.oscar.aol.com", 9898 )

class TOCError(Exception):
	pass

class TOCDisconnectError(Exception):
	pass

class TocTalk:
	def __init__(self,nick,passwd):
		self._nick = nick
		self._passwd = passwd
		self._agent = "BnIRC aim plugin (based on PY-TOC)"
		self._info = "BnIRC - Smack my BitchX up"
		self._seq = whrandom.randint(0,65535)
		self._logfd = sys.stdout
		self._debug = 1
		self._running = 0
		self._ignore = 0
		self.build_funcs()


	def build_funcs(self):
		self._dir = []
		for item in dir(self.__class__):
			if ( type( eval("self.%s" % item)) == type(self.__init__) and
			item[:3] == "on_" ):
				self._dir.append(item)


	def go(self):
		print "connecting to AIM"
		self.connect()
		bnirc.RegisterPoll(self._socket.fileno(), self.process_loop)

	def start(self):
		pass

	def connect(self):
		#create the socket object
		try:
			self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		except:
			raise TOCError, "FATAL:  Couldn't create a socket"

		# make the connection
		try:
			self._socket.connect( TOC_SERV )
		except:
			raise TOCDisconnectError, "FATAL: Could not connect to TOC Server"

		buf  = "FLAPON\r\n\r\n"
		bsent = self._socket.send(buf)
		#self._socket.setblocking(False)

		if bsent <> len(buf):
			raise TOCError, "FATAL: Couldn't send FLAPON!"

		print "Connected to AIM (TOC)"

	def start_log_in(self):
		ep = self.pwdenc()
		self._normnick = self.normalize(self._nick)
		msg = struct.pack("!HHHH",0,1,1,len(self._normnick)) + self._normnick
		self.flap_to_toc(1,msg)

		#now, login
		self.flap_to_toc(2,"toc_signon %s %s %s %s english %s" % (
		TOC_SERV_AUTH[0],TOC_SERV_AUTH[1],self._normnick,ep,self.encode(self._agent) ) )


	def normalize(self,data):
		return  re.sub("[^A-Za-z0-9]","",data).lower()

	def encode(self,data):
		for letter in "\\(){}[]$\"":
			data = data.replace(letter,"\\%s"%letter)
		return '"' + data + '"'

	def flap_to_toc(self,ftype,msg):
		if ftype == 2:
			msg = msg + struct.pack("!B", 0)
		ditems = []
		ditems.append("*")
		ditems.append(struct.pack("!BHH",ftype,self._seq,len(msg)))
		ditems.append(msg)


		data = "".join(ditems)

		if len(data) >= 2048:
			raise TOCError, "TOC data with protocol overhead cannot exceed 2048 bytes."

		self.derror( "SEND : \'%r\'" % data )


		# in case we're threading
		bsent = self._socket.send(data)


		if bsent <> len(data):
			#maybe make less severe later
			# I've never seen this happen.. have you??
			raise TOCError, "FATAL: Couldn't send all data to TOC Server\n"

		self._seq = self._seq + 1

	def pwdenc(self):
		lookup = "Tic/Toc"
		ept = []

		x = 0
		for letter in self._passwd:
			ept.append("%02x" % ( ord(letter) ^ ord( lookup[x % 7]) ) )
			x = x + 1
		return "0x" + "".join(ept)

	def process_loop(self):
		# the "main" loop
		try:
			event = self.recv_event()
			if not event:
				return
			self.handle_event(event)
		except TOCDisconnectError:
			self.connect()

	def handle_event(self,event):

		self.derror( "RECV : %r" % event[1] )

		#else, fig out what to do with it
		#special case-- login
		if event[0] == 1:
			self.start_log_in()
			return

		if not event[1].count(":"):
			data = ""
			id = "NOID"

		else:
			ind = event[1].find(":")
			id = event[1][:ind].upper()
			data = event[1][ind+1:]

		#handle manually now
		if id == "SIGN_ON":
			self.c_SIGN_ON(id,data)
			return

		if id == "ERROR":
			self.c_ERROR(id,data)
			return

		#their imp
		if ("on_%s" % id ) in self._dir:
			exec ( "self.on_%s(data)" % id )

		else:
			self.werror("INFO : Received unimplemented '%s' id" % id)

	def recv_event(self):
		header = self._socket.recv(6)

		if header == "":
			self.err_disconnect()
			return

		(marker,mtype,seq,buflen) = struct.unpack("!sBhh",header)

		#get the info
		dtemp = self._socket.recv(buflen)
		data = dtemp
		while len(data) != buflen:
			if dtemp == "":
				self.err_disconnect()
				return
			dtemp = self._socket.recv(buflen - len(data))
			data = data + dtemp

		return (mtype, data)

	def thread_recv_events(self):
		while self._running:
			rfd,dc,dc = select.select([self._socket],[],[])
			if rfd == []:
				continue
			try:
				header = self._socket.recv(6)
			except:
				self.err_disconnect()

			if header == "":
				self.err_disconnect()

			(marker,mtype,seq,buflen) = struct.unpack("!sBhh",header)

			#get the info
			dtemp = self._socket.recv(buflen)
			data = dtemp
			while len(data) != buflen:
				if dtemp == "":
					self.err_disconnect()
				dtemp = self._socket.recv(buflen - len(data))
				data = data + dtemp

			if not self._ignore:
				self.handle_event([mtype,data])

	def err_disconnect(self):
		self.werror( "INFO: Disconnected!\n" )
		raise TOCDisconnectError, "FATAL: We seem to have been disconnected from the TOC server.\n"

	# our event handling
	def c_ERROR(self,id,data):
		# let's just grab the errors we care about!

		#still more fields
		if data.count(":"):
			dt = int (data[:data.find(":")])
		else:
			dt = int(data) # let's get an int outta it

		if dt == 980:
			raise TOCError, "FATAL: Couldn't sign on; Incorrect nickname/password combination"

		elif dt == 981:
			raise TOCError, "FATAL: Couldn't sign on; The AIM service is temporarily unavailable"

		elif dt == 982:
			raise TOCError, "FATAL: Couldn't sign on; Your warning level is too high"

		elif dt == 983:
			raise TOCError, "FATAL: Couldn't sign on; You have been connecting and disconnecting too frequently"

		elif dt == 989:
			raise TOCError, "FATAL: Couldn't sign on; An unknown error occurred"

		# ... etc etc etc
		else:
			# try to let further implementation handle it
			if ("on_%s" % id ) in self._dir:
				exec ( "self.on_%s(data)" % id )
			else:
				self.werror("ERROR: The TOC server sent an unhandled error string: %s" % data)

	def c_SIGN_ON(self,type,data):
		self.flap_to_toc(2,"toc_add_buddy %s" % self.normalize(self._nick)) # needs to start up corectly
		self.flap_to_toc(2,"toc_set_info %s" % self.encode(self._info) )
		self.flap_to_toc(2,"toc_init_done")
		self.start()

	def strip_html(self,data):
		data = re.sub("<[^>]*>","",data)
		data = re.sub(r'[&]amp[;]', r'&', data)
		data = re.sub(r'[&]quot[;]', r'"', data)
		data = re.sub(r'[&][#]039[;]', r'\\', data)
		data = re.sub(r'[&]lt[;]', r'<', data)
		data = re.sub(r'[&]gt[;]', r'>', data)
		return data

	def normbuds(self,buddies):
		nbuds = []
		for buddy in buddies:
			nbuds.append(self.normalize(buddy))
		return " ".join(nbuds)

	#actions--help the user w/common tasks

	#the all-important
	def do_SEND_IM(self,user,message,autoaway=0):
		sendmessage = "toc_send_im %s %s" % ( self.normalize(user), self.encode(message) )
		if autoaway:
			sendmessage = sendmessage + " auto"
		self.flap_to_toc(2, sendmessage)

	def do_ADD_BUDDY(self,buddies):
		self.flap_to_toc(2,"toc_add_buddy %s" % self.normbuds(buddies) )

	def do_ADD_PERMIT(self,buddies):
		self.flap_to_toc(2,"toc_add_permit %s" % self.normbuds(buddies) )

	def do_ADD_DENY(self,buddies):
		self.flap_to_toc(2,"toc_add_deny %s" % self.normbuds(buddies) )

	def do_REMOVE_BUDDY(self,buddies):
		self.flap_to_toc(2,"toc_remove_buddy %s" % self.normbuds(buddies) )

	# away, idle, user info handling
	def do_SET_IDLE(self,itime):
		self.flap_to_toc(2,"toc_set_idle %d" % itime )

	def do_SET_AWAY(self,awaymess):
		if awaymess == "":
			self.flap_to_toc(2,"toc_set_away")
			return

		self.flap_to_toc(2,"toc_set_away %s" % self.encode(awaymess) )

	def do_GET_INFO(self,user):
		self.flap_to_toc(2,"toc_get_info %s" % self.normalize(user) )

	def do_SET_INFO(self,info):
		self.flap_to_toc(2,"toc_set_info %s" % self.encode(info) )

	# warning capability
	def do_EVIL(self,user,anon=0):
		if anon:
			acode = "anon"
		else:
			acode = "norm"

		self.flap_to_toc(2,"toc_evil %s %s" % (self.normalize(user), acode) )

	#chat
	def do_CHAT_INVITE(self,room,imess,buddies):
		self.flap_to_toc(2,"toc_chat_invite %s %s %s" % (self.normalize(room),
		self.encode(imess), self.normbuds(buddies) ) )

	def do_CHAT_ACCEPT(self, id):
		self.flap_to_toc(2,"toc_chat_accept %s" % id)

	def do_CHAT_LEAVE(self,id):
		self.flap_to_toc(2,"toc_chat_leave %s" % id)

	def do_CHAT_WHISPER(self,room,user,message):
		self.flap_to_toc(2,"toc_chat_whisper %s %s %s" % (room,
		self.normalize(user), self.encode(message) ) )

	def do_CHAT_SEND(self,room,message):
		self.flap_to_toc(2,"toc_chat_send %s %s" % (room,
		self.encode(message) ) )

	def do_CHAT_JOIN(self,roomname):
		self.flap_to_toc(2,"toc_chat_join 4 %s" % roomname)

	def do_SET_CONFIG(self,configstr):
		self.flap_to_toc(2,"toc_set_config \"%s\"" % configstr)

	# error funcs
	def werror(self,errorstr):
		if self._debug:
			self._logfd.write("(%s) %s\n"% (self._nick,errorstr))

	def derror(self,errorstr):
		if self._debug > 1:
			self._logfd.write("(%s) %s\n"% (self._nick,errorstr))

class BnAim(TocTalk):

	def on_IM_IN(self, data):
		data = data.split(':', 2)
		data[2] = self.strip_html(data[2]);
		bnirc.IncomingMsg("AimFrom(%s)" % (data[0]), data[2])
		bnirc.HandleAway(data[0], "AIM", data[2])
		if bnirc.IsAway():
			self.do_SEND_IM(data[0], bnirc.StringOption("away_string"))

	def buddy_add(self, args):
		self.do_ADD_BUDDY([args[1]])
		self.config += "b %s\n" % (args[1])
		self.do_SET_CONFIG(self.config)

	def buddy_remove(self, args):
		tmp = self.config.split("\n")
		config = ""
		for i in tmp:
			j = i.split()
			if len(j) > 1 and j[1].upper() != args[1].upper():
				self.config += i + "\n"
		self.do_SET_CONFIG(config)
		self.do_REMOVE_BUDDY([args[1]])
		self.config = config

	def on_UPDATE_BUDDY(self, data):
		if type(self.buds) != type({}):
			self.buds = {}
		data = data.split(':')
		if len(data) < 2:
			print "evil Buddy Update from server!"
			print "".join(data)
			return -1
		if data[1] == "F":
			self.buds.__delitem__(data[0])
			print data[0] + " has left AIM"
		else:
			if data[1] == "T" and self.buds.has_key(data[0]) == False:
				self.buds[data[0]] = True
				bnirc.UserMsg(data[0] + " is now on AIM\n")

	def on_CONFIG(self, data):

		self.config = data

		if len(self.buds) == 0:
			buds = []

			for item in data.split("\n"):
				if item == '':
					continue
				if item[0] == "b":
					buds.append(item[1:].strip())

				# can't add more then 20 at once
				# becasue of len
				if len(buds) == 20:
					self.do_ADD_BUDDY(buds)
					time.sleep(0.2)
					buds = []
			if len(buds):
				self.do_ADD_BUDDY(buds)

	def __init__(self, nick, passw):
		self.config = ""
		self.cfgset = None
		self.buds = {}
		TocTalk.__init__(self, nick, passw)

class BnIRCApp:

	def connect ( self, args ):
		self.aim = BnAim(args[1], args[2])
		self.aim._debug = 0
		self.aim.go()
		return 0

	def htmlspecialchars(self, str):
		rv = re.sub('&', '&amp;', str)
		rv = re.sub('"', '&quot;', rv)
		#rv = re.sub('\'', '&#039;', rv)
		rv = re.sub('<', '&lt;', rv)
		rv = re.sub('>', '&gt;', rv)
		return rv

	def send ( self, args ):
		if self.aim is None:
			return 1
		bnirc.IncomingMsg("AimTo(%s)" % (args[1]), args[2])
		self.aim.do_SEND_IM(args[1], self.htmlspecialchars(args[2]))
		return 0

	def who ( self, args ):
		if self.aim is None:
			return -1
		if type(self.aim.buds) != type({}):
			print "no buddies online!"
			return 0
		for i in self.aim.buds.keys():
			bnirc.UserMsg("[%s]\n" % (i))
		return 0

	def add ( self, args ):
		if not self.aim:
			bnirc.UserMsg("please aim:connect first!")
			return 0
		self.aim.buddy_add(args)
		return 0

	def remove (self, args):
		if not self.aim:
			bnirc.UserMsg("please aim:connect first!")
			return 0
		self.aim.buddy_remove(args)
		self.aim.buds.__delitem__(args[1])
		return 0

	def tab_complete ( self, partial ):
		if not self.aim:
			return []
		sz = len(partial)
		rv = []
		for i in self.aim.buds.keys():
			if i[:sz].upper() == partial.upper():
				if len(i.split()) > 1:
					rv.append("\"%s\"" % (i))
				else:
					rv.append(i)
		return rv

	def __init__(self):
		self.aim = None
		bnirc.RegisterCommand("aim:connect", "aim:connect <username> <password>", "connect to aim", 3, 3, self.connect)
		bnirc.RegisterCommand("aim", "aim <username> <message>", "send a message to aim dude", 3, 3, self.send)
		bnirc.RegisterCommand("aim:who", "aim:who", "see your online buddies", 1, 1, self.who)
		bnirc.RegisterCommand("aim:add", "aim:add <username> [group]", "add someone to your buddy list", 2, 3, self.add)
		bnirc.RegisterCommand("aim:remove", "aim:remove <username>", "remove someone from your buddy list", 2, 2, self.remove)
		bnirc.RegisterTab(self.tab_complete)

app = BnIRCApp()
