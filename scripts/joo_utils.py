#!/usr/bin/env python

###########################################################################
#   Copyright (C) 2005 by Janne Oksanen                                   #
#   jaosoksa@cc.jyu.fi                                                    #
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

# This is a small python script for bnIRC including some features I thought
# would come in handy. This is mostly stuff I missed when chatting with North
# American people with their completely random measure units. This is written
# very much from a European point of view, but I hope it can be of some use
# for other people too.
# -Janne
 
import bnirc

# Just some currency convertion rates respective to euros. (Bank of Finland, 11.8.2005, 15:15)

USD_RATE = 1.2405
CAD_RATE = 1.4976
AUD_RATE = 1.6103
SEK_RATE = 9.3223
EEK_RATE = 15.6466
GBP_RATE = 0.68825
JPY_RATE = 136.94

##### IMPERIALS TO SI-UNITS CONVESIONS #####

def f2c(args):
	temp = (float(args[1]) - 32) * 5/9
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " C")
	return 0
 
def c2f(args):
	temp = (float(args[1]) * 9/5) + 32
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " F")
	return 0

def lb2kg(args):
	temp = float(args[1]) / float(2.20462262)
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " kg")
	return 0

def kg2lb(args):
	temp = float(args[1]) * float(2.20462262)
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " lb")
	return 0 

def mi2km(args):
	temp = float(args[1]) * float(1.609344)
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " km")
	return 0

def km2mi(args):
	temp = float(args[1]) / float(1.609344)
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " mi")
	return 0

def in2cm(args):
	temp = float(args[1]) / float(2.54)
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " cm")
	return 0

def cm2in(args):
	temp = float(args[1]) / float(2.54)
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + '"')
	return 0
	
def ft2m(args):
	temp = float(args[1]) * float(0.3048)
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " m")
	return 0

def m2ft(args):
	temp = float(args[1]) / float(0.3048)
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + "'")
	return 0

def gal2l(args):
	temp = float(args[1]) * float(4.54609)
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " L")
	return 0

def l2gal(args):
	temp = float(args[1]) / float(4.54609)
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " gal")
	return 0


##### EUROS TO OTHERS CONVERSIONS #####

def eur2usd(args):
	temp = float(args[1]) * USD_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " USD")
	return 0

def usd2eur(args):
	temp = float(args[1]) / USD_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " EUR")
	return 0

def eur2cad(args):
	temp = float(args[1]) * CAD_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " CAD")
	return 0

def cad2eur(args):
	temp = float(args[1]) / CAD_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " EUR")
	return 0

def eur2aud(args):
	temp = float(args[1]) * AUD_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " AUD")
	return 0

def aud2eur(args):
	temp = float(args[1]) / AUD_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " EUR")
	return 0

def eur2gbp(args):
	temp = float(args[1]) * GBP_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " GBP")
	return 0

def gbp2eur(args):
	temp = float(args[1]) / GBP_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " EUR")
	return 0

def eur2jpy(args):
	temp = float(args[1]) * JPY_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " JPY")
	return 0

def jpy2eur(args):
	temp = float(args[1]) / JPY_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " EUR")
	return 0

def eur2eek(args):
	temp = float(args[1]) * EEK_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " EEK")
	return 0

def eek2eur(args):
	temp = float(args[1]) / EEK_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " EUR")
	return 0

def eur2sek(args):
	temp = float(args[1]) * SEK_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " SEK")
	return 0

def sek2eur(args):
	temp = float(args[1]) / SEK_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " EUR")
	return 0

##### USD TO OTHERS CONVERSIONS #####

def usd2cad(args):
	temp = (float(args[1]) / USD_RATE) * CAD_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " CAD")
	return 0

def cad2usd(args):
	temp = (float(args[1]) / CAD_RATE) * USD_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " USD")
	return 0

def usd2aud(args):
	temp = (float(args[1]) / USD_RATE) * AUD_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " AUD")
	return 0

def aud2usd(args):
	temp = (float(args[1]) / AUD_RATE) * USD_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " USD")
	return 0
	
def usd2gbp(args):
	temp = (float(args[1]) / USD_RATE) * GBP_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " GBP")
	return 0

def gbp2usd(args):
	temp = (float(args[1]) / GBP_RATE) * USD_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " USD")
	return 0

def usd2jpy(args):
	temp = (float(args[1]) / USD_RATE) * JPY_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " JPY")
	return 0

def jpy2usd(args):
	temp = (float(args[1]) / JPY_RATE) * USD_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " USD")
	return 0

def usd2eek(args):
	temp = (float(args[1]) / USD_RATE) * EEK_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " EEK")
	return 0

def eek2usd(args):
	temp = (float(args[1]) / EEK_RATE) * USD_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " USD")
	return 0

def usd2sek(args):
	temp = (float(args[1]) / USD_RATE) * SEK_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " SEK")
	return 0

def sek2usd(args):
	temp = (float(args[1]) / SEK_RATE) * USD_RATE
	temp = round(temp, 2)
	bnirc.IrcSay(str(temp) + " USD")
	return 0


##### P E G I N A #####

def pegina(args):
	bnirc.IrcSay(" ****  *****  ****  **** **   **    **    ")
	bnirc.IrcSay(" ** ** **    **  **  **  ***  **   ****   ")
        bnirc.IrcSay(" ****  ****  **      **  **** **  **  **  ")
	bnirc.IrcSay(" **    **    ** ***  **  **  *** ******** ")
	bnirc.IrcSay(" **    *****  ****  **** **   ** **    ** ")
	return 0


##### MEASURES ##### 
bnirc.RegisterCommand("f2c", "f2c <temperature in F>", "Converts Celcius temperature to Fahrenheit.", 2, 2, f2c)
bnirc.RegisterCommand("c2f", "c2f <temperature in C>", "Converts Fahrenheit temperature to Celcius.", 2, 2, c2f)
bnirc.RegisterCommand("lb2kg", "lb2kg <mass in lb>", "Converts mass given in silly hobbit units to SI-units.", 2, 2, lb2kg)
bnirc.RegisterCommand("kg2lb", "kg2lb <mass in kg>", "Converts SI-standard mass to silly inferials.", 2, 2, kg2lb)
bnirc.RegisterCommand("mi2km", "mi2km <distance in miles>", "Converts miles to kilometers.", 2, 2, mi2km)
bnirc.RegisterCommand("km2mi", "km2mi <distance in km>", "Converts kilometers to miles.", 2, 2, km2mi)
bnirc.RegisterCommand("in2cm", "in2cm <length in inches>", "Converts inches to centimeters.", 2, 2, in2cm)
bnirc.RegisterCommand("cm2in", "cm2in <length in cm>", "Converts centimeters to inches.", 2, 2, cm2in)
bnirc.RegisterCommand("ft2m", "ft2m <distance in feet>", "Converts feet to meters.", 2, 2, ft2m)
bnirc.RegisterCommand("m2ft", "m2ft <distance in meters>", "Converts meters to feet.", 2, 2, m2ft)
bnirc.RegisterCommand("gal2l", "gal2l <volume in gallons>", "Converts gallons to litres.", 2, 2, gal2l)
bnirc.RegisterCommand("l2gal", "l2gal <volume in litres>", "Converts litres to gallons.", 2, 2, l2gal)

##### EURO CONVERSIONS #####
bnirc.RegisterCommand("eur2usd", "eur2usd <value in EUR>", "Converts euros to US dollars.", 2, 2, eur2usd)
bnirc.RegisterCommand("usd2eur", "usd2eur <value in USD>", "Converts US dollars to euros.", 2, 2, usd2eur)
bnirc.RegisterCommand("eur2cad", "eur2cad <value in EUR>", "Converts euros to Canadian dollars.", 2, 2, eur2cad)
bnirc.RegisterCommand("cad2eur", "cad2eur <value in CAD>", "Converts Canadian dollars to euros.", 2, 2, cad2eur)
bnirc.RegisterCommand("eur2aud", "eur2aud <value in EUR>", "Converts euros to Australian dollars.", 2, 2, eur2aud)
bnirc.RegisterCommand("aud2eur", "aud2eur <value in AUD>", "Converts Australian dollars to euros.", 2, 2, aud2eur)
bnirc.RegisterCommand("eur2gbp", "eur2gbp <value in EUR>", "Converts euros to British pounds.", 2, 2, eur2gbp)
bnirc.RegisterCommand("gbp2eur", "gbp2eur <value in GBP>", "Converts British pounds to euros.", 2, 2, gbp2eur)
bnirc.RegisterCommand("eur2jpy", "eur2jpy <value in EUR>", "Converts euros to yens.", 2, 2, eur2jpy)
bnirc.RegisterCommand("jpy2eur", "jpy2eur <value in JPY>", "Converts yens to euros.", 2, 2, jpy2eur)
bnirc.RegisterCommand("eur2eek", "eur2eek <value in EUR>", "Converts euros to Estonian crowns.", 2, 2, eur2eek)
bnirc.RegisterCommand("eek2eur", "eek2eur <value in EEK>", "Converts Estonian crowns to euros.", 2, 2, eek2eur)
bnirc.RegisterCommand("eur2sek", "eur2sek <value in EUR>", "Converts euros to Swedish crowns.", 2, 2, eur2sek)
bnirc.RegisterCommand("sek2eur", "sek2eur <value in SEK>", "Converts Swedish crowns to euros.", 2, 2, sek2eur)

##### USD CONVERSIONS #####
bnirc.RegisterCommand("usd2cad", "usd2cad <value in USD>", "Converts US dollars  to Canadian dollars.", 2, 2, usd2cad)
bnirc.RegisterCommand("cad2usd", "cad2usd <value in CAD>", "Converts Canadian dollars to US dollars.", 2, 2, cad2usd)
bnirc.RegisterCommand("usd2aud", "usd2aud <value in USD>", "Converts US dollars  to Australian dollars.", 2, 2, usd2aud)
bnirc.RegisterCommand("cad2usd", "cad2usd <value in CAD>", "Converts Canadian dollars to US dollars.", 2, 2, aud2usd)
bnirc.RegisterCommand("usd2gbp", "usd2gbp <value in USD>", "Converts US dollars  to British pounds.", 2, 2, usd2gbp)
bnirc.RegisterCommand("gbp2usd", "gbp2usd <value in GBP>", "Converts British pounds to US dollars.", 2, 2, gbp2usd)
bnirc.RegisterCommand("usd2jpy", "usd2jpy <value in USD>", "Converts US dollars to yens.", 2, 2, usd2jpy)
bnirc.RegisterCommand("jpy2usd", "jpy2usd <value in JPY>", "Converts yens to US dollars.", 2, 2, jpy2usd)
bnirc.RegisterCommand("usd2eek", "usd2eek <value in USD>", "Converts US dollars to Estonian crowns.", 2, 2, usd2eek)
bnirc.RegisterCommand("eek2usd", "eek2usd <value in EEK>", "Converts Estonian crowns to US dollars.", 2, 2, eek2usd)
bnirc.RegisterCommand("usd2sek", "usd2sek <value in SEK>", "Converts US dollars to Swedish crowns.", 2, 2, usd2sek)
bnirc.RegisterCommand("sek2usd", "sek2usd <value in SEK>", "Converts Swedish crowns to US dollars.", 2, 2, sek2eur)


##### P E G I N A #####
bnirc.RegisterCommand("pegina", "pegina", "This is here just for flooding.", 1, 1, pegina)

 
bnirc.DebugMsg("joo_utils script loaded")
