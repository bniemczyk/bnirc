import bnirc
import sys

class __bnircOut:
	def write(self, str):
		bnirc.DebugMsg(str)

__bnirc_out = __bnircOut()
sys.stdout = __bnirc_out
sys.stderr = __bnirc_out

class BnircCommands:


	def tab_complete ( self, partial ):
		sz = len(partial)
		rv = []
		for i in self.completes:
			if i[:sz].upper() == partial.upper():
				rv.append(i)
		return rv

	def tab_add ( self, com ):
		if self.completes.count(com[1]):
			bnirc.UserMsg("%s is already in your tab complete list!" % (com[1]))
			return 0
		self.completes.append(com[1])
		return 0

	def tab_rem ( self, com ):
		if not self.completes.count(com[1]):
			bnirc.UserMsg("%s is not in your tab complete list!" % (com[1]))
			return 0
		self.completes.remove(com[1])
		return 0

	def python_help ( self, args ):
		help(bnirc)
		return 0

	def identify (self, args):
		bnirc.IrcRaw("PRIVMSG NickServ :identify %s" % (args[1]))
		return 0

	def myip (self, args):
		bnirc.UserMsg("my ip address is: %s" % (bnirc.CurrentIp()))
		return 0

	def __init__ (self):
		self.completes = []
		bnirc.RegisterCommand("tab:add", "tab:add <string>",
				"add a string to your tab completion list", 2, 2, self.tab_add)
		bnirc.RegisterCommand("tab:del", "tab:del <string>",
				"remove a string from your tab completion list", 2, 2, self.tab_rem)
		bnirc.RegisterCommand("py:help", "py:help",
				"shows python bnirc module help", 1, 1, self.python_help)
		bnirc.RegisterCommand("identify", "identify <passwd>",
				"same as /msg NickServ identify <passwd>, but it doesn't show the msg" +
				", so your password is secure", 2, 2, self.identify)
		bnirc.RegisterCommand("myip", "myip",
				"show your current ip", 1, 1, self.myip)
		bnirc.RegisterTab(self.tab_complete)

commands = BnircCommands()
