# script to help a user create a basic .bnirc file
#

import bnirc
import os

home_dir = os.getenv("HOME")
init_file = home_dir + "/.bnirc"

def mkifile():
	bnirc.ColoredMsg("It appears you do not have a .bnirc file.  We will create one now\n", bnirc.USER_WHITE)
	nick = bnirc.Ask("What nick name would you like to use?")
	server = bnirc.Ask("What IRC server would you like to connect to?")
	channels = bnirc.Ask("What channels would you like to join (seperate with spaces)?").split()

	def fix_channel(c):
		if c[0] == '#': return c
		return "#" + c

	channels = [fix_channel(c) for c in channels]
	seperate = bnirc.Ask("Would you like channels in seperate windows, or in one common window (enter 'common' for 1 window, anything else for seperate)?")

	f = open(init_file, "w")
	f.write('''# bnIRC initialization file
# for help visit #bnirc on irc.freenode.net\n\n
''')
	bnirc.SetOption("default_nick", nick)
	for s in bnirc.GetSettings():
		val = bnirc.StringOption(s)
		if val == "" or val == None:
			f.write("# /set %s\n" % (s))
		else:
			f.write("/set %s %s\n" % (s, bnirc.StringOption(s)))

	f.write("\n/server %s\n" % (server))

	win = 1
	for i in channels:
		f.write("\n/window %d\n" % win)
		f.write("/join %s\n" % i)
		if seperate != "common\n": win += 1

	f.close()

	bnirc.ParseInput("source %s" % (init_file))
	bnirc.ColoredMsg("Don't forget to modify %s later to fit your needs\n" % init_file, bnirc.USER_WHITE)
	bnirc.ColoredMsg("You can get help at ", bnirc.USER_WHITE)
	bnirc.ColoredMsg("#bnirc on irc.freenode.net\n", bnirc.USER_BLUE)

try:
	is_there = open(init_file)
	is_there.close()
except:
	mkifile()
