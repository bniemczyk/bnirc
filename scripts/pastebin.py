# this is a pastebin script for bnirc, to use it
# place the script in ~/.bnIRC/scripts and add "/python <this scripts filename>"
# to your .bnirc file

import bnirc
import urllib
import urllib2
import re

def paste(content, description="", name="", expiry="", type="raw"):
    TYPES = [
            "raw", "asterisk", "c", "cpp", "php", "perl", "java", "vb", "csharp",
            "ruby", "python", "pascal", "mirc", "pli", "xml", "sql", "scheme",
            "ascript", "ada", "apache", "nasm", "asp", "bash", "css", "delphi", "html", "js",
            "lisp", "lua", "asm", "objc", "vbnet" ]

    POSTDATA = "content=%(content)s&description=%(description)s&type=%(type)s&expiry=%(expiry)s&name=%(name)s&save=0&s=Submit+Post"
    URL = "http://en.pastebin.ca/index.php"
    URL_PATTERN = re.compile('http://en.pastebin.ca/\d+')

    payload = POSTDATA % {
            "content" : urllib.quote_plus(content),
            "description" : urllib.quote_plus(description),
            "type" : TYPES.index(type)+1,
            "expiry" : urllib.quote_plus(expiry),
            "name" : urllib.quote_plus(name)
            }

    req = urllib2.Request(URL, payload, {
        "Referer": "http://en.pastebin.ca/",
        "Content-Type": "application/x-www-form-urlencoded",
        "User-Agent": "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.7.10) Gecko/20050813 Epiphany/1.7.6",
        "Accept-Language": "en",
        "Accept-Charset": "utf-8",
        })

    html = urllib2.urlopen(req).read()
    stuff = re.search('content=["]7;([^"]*)', html, re.MULTILINE)
    return stuff.group(1)

def paste_file(fname, description="", name="", expiry="", type="raw"):
    f = open(fname)
    contents = f.read()
    f.close()
    return paste(contents, description, name, expiry, type)

def bnirc_paste_file(args):
    fname = args[1]
    url = paste_file(fname)
    bnirc.IrcSay(url)
    return 0

def bnirc_paste_cmd(args):
    f = os.popen(args[1])
    contents = f.read()
    f.close()
    url = paste(contents)
    bnirc.IrcSay(url)
    return 0

bnirc.RegisterCommand("pastebin", "pastebin <filename>", "post a file to a pastebin and display the url in the current channel/query", 2, 2, bnirc_paste_file)

bnirc.RegisterCommand("pastebin-cmd", "pastebin-cmd <shell command>", "post the output of a shell command to a pastebin and display the url in the current channel/query", 2, 2, bnirc_paste_cmd)

bnirc.DebugMsg("pastebin script loaded")
