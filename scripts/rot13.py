#!/usr/bin/env python

###########################################################################
#   Copyright (C) 2005 by Andrew Riedi                                    #
#   andrew@snprogramming.com                                              #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
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

import bnirc

def decode_rot13(args):
	out = ""

	if str(args[3][0:4]) == "r13 ":
		string = str(args[3][4:])
	
		length = len(string)
		i = j = 0
		while j < length:
			i = j
			j = j + 1
			if string[i:j] == "a":
				out = out + "n"
			elif string[i:j] == "b":
				out = out + "o"
			elif string[i:j] == "c":
				out = out + "p"
			elif string[i:j] == "d":
				out = out + "q"
			elif string[i:j] == "e":
				out = out + "r"
			elif string[i:j] == "f":
				out = out + "s"
			elif string[i:j] == "g":
				out = out + "t"
			elif string[i:j] == "h":
				out = out + "u"
			elif string[i:j] == "i":
				out = out + "v"
			elif string[i:j] == "j":
				out = out + "w"
			elif string[i:j] == "k":
				out = out + "x"
			elif string[i:j] == "l":
				out = out + "y"
			elif string[i:j] == "m":
				out = out + "z"
			elif string[i:j] == "n":
				out = out + "a"
			elif string[i:j] == "o":
				out = out + "b"
			elif string[i:j] == "p":
				out = out + "c"
			elif string[i:j] == "q":
				out = out + "d"
			elif string[i:j] == "r":
				out = out + "e"
			elif string[i:j] == "s":
				out = out + "f"
			elif string[i:j] == "t":
				out = out + "g"
			elif string[i:j] == "u":
				out = out + "h"
			elif string[i:j] == "v":
				out = out + "i"
			elif string[i:j] == "w":
				out = out + "j"
			elif string[i:j] == "x":
				out = out + "k"
			elif string[i:j] == "y":
				out = out + "l"
			elif string[i:j] == "z":
				out = out + "m"
			elif string[i:j] == "A":
				out = out + "N"
			elif string[i:j] == "B":
				out = out + "O"
			elif string[i:j] == "C":
				out = out + "P"
			elif string[i:j] == "D":
				out = out + "Q"
			elif string[i:j] == "E":
				out = out + "R"
			elif string[i:j] == "F":
				out = out + "S"
			elif string[i:j] == "G":
				out = out + "T"
			elif string[i:j] == "H":
				out = out + "U"
			elif string[i:j] == "I":
				out = out + "V"
			elif string[i:j] == "J":
				out = out + "W"
			elif string[i:j] == "K":
				out = out + "X"
			elif string[i:j] == "L":
				out = out + "Y"
			elif string[i:j] == "M":
				out = out + "Z"
			elif string[i:j] == "N":
				out = out + "A"
			elif string[i:j] == "O":
				out = out + "B"
			elif string[i:j] == "P":
				out = out + "C"
			elif string[i:j] == "Q":
				out = out + "D"
			elif string[i:j] == "R":
				out = out + "E"
			elif string[i:j] == "S":
				out = out + "F"
			elif string[i:j] == "T":
				out = out + "G"
			elif string[i:j] == "U":
				out = out + "H"
			elif string[i:j] == "V":
				out = out + "I"
			elif string[i:j] == "W":
				out = out + "J"
			elif string[i:j] == "X":
				out = out + "K"
			elif string[i:j] == "Y":
				out = out + "L"
			elif string[i:j] == "Z":
				out = out + "M"
			else:
				out = out + string[i:j]
		
	bnirc.UserMsg(out)

	return 1

def encode_rot13(args):
	rot13 = str(args[1])
	length = len(rot13)
	i = j = 0
	out = ""

	bnirc.UserMsg(rot13)
	
	while j < length:
		i = j
		j = j + 1
		if rot13[i:j] == "a":
			out = out + "n"
		elif rot13[i:j] == "b":
			out = out + "o"
		elif rot13[i:j] == "c":
			out = out + "p"
		elif rot13[i:j] == "d":
			out = out + "q"
		elif rot13[i:j] == "e":
			out = out + "r"
		elif rot13[i:j] == "f":
			out = out + "s"
		elif rot13[i:j] == "g":
			out = out + "t"
		elif rot13[i:j] == "h":
			out = out + "u"
		elif rot13[i:j] == "i":
			out = out + "v"
		elif rot13[i:j] == "j":
			out = out + "w"
		elif rot13[i:j] == "k":
			out = out + "x"
		elif rot13[i:j] == "l":
			out = out + "y"
		elif rot13[i:j] == "m":
			out = out + "z"
		elif rot13[i:j] == "n":
			out = out + "a"
		elif rot13[i:j] == "o":
			out = out + "b"
		elif rot13[i:j] == "p":
			out = out + "c"
		elif rot13[i:j] == "q":
			out = out + "d"
		elif rot13[i:j] == "r":
			out = out + "e"
		elif rot13[i:j] == "s":
			out = out + "f"
		elif rot13[i:j] == "t":
			out = out + "g"
		elif rot13[i:j] == "u":
			out = out + "h"
		elif rot13[i:j] == "v":
			out = out + "i"
		elif rot13[i:j] == "w":
			out = out + "j"
		elif rot13[i:j] == "x":
			out = out + "k"
		elif rot13[i:j] == "y":
			out = out + "l"
		elif rot13[i:j] == "z":
			out = out + "m"
		elif rot13[i:j] == "A":
			out = out + "N"
		elif rot13[i:j] == "B":
			out = out + "O"
		elif rot13[i:j] == "C":
			out = out + "P"
		elif rot13[i:j] == "D":
			out = out + "Q"
		elif rot13[i:j] == "E":
			out = out + "R"
		elif rot13[i:j] == "F":
			out = out + "S"
		elif rot13[i:j] == "G":
			out = out + "T"
		elif rot13[i:j] == "H":
			out = out + "U"
		elif rot13[i:j] == "I":
			out = out + "V"
		elif rot13[i:j] == "J":
			out = out + "W"
		elif rot13[i:j] == "K":
			out = out + "X"
		elif rot13[i:j] == "L":
			out = out + "Y"
		elif rot13[i:j] == "M":
			out = out + "Z"
		elif rot13[i:j] == "N":
			out = out + "A"
		elif rot13[i:j] == "O":
			out = out + "B"
		elif rot13[i:j] == "P":
			out = out + "C"
		elif rot13[i:j] == "Q":
			out = out + "D"
		elif rot13[i:j] == "R":
			out = out + "E"
		elif rot13[i:j] == "S":
			out = out + "F"
		elif rot13[i:j] == "T":
			out = out + "G"
		elif rot13[i:j] == "U":
			out = out + "H"
		elif rot13[i:j] == "V":
			out = out + "I"
		elif rot13[i:j] == "W":
			out = out + "J"
		elif rot13[i:j] == "X":
			out = out + "K"
		elif rot13[i:j] == "Y":
			out = out + "L"
		elif rot13[i:j] == "Z":
			out = out + "M"
		else:
			out = out + rot13[i:j]
	
	bnirc.IrcSay("r13 " + out)
	
	return 0

bnirc.RegisterServerString("privmsg", 1, 4, decode_rot13)

bnirc.RegisterCommand("rot13", "rot13 <text to change>", "Rotates all letters by 13.", 2, 2, encode_rot13)
 
bnirc.DebugMsg("rot13 script loaded")
