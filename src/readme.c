extern const char README[]; // make sure it gets exported
const char README[] = 
"you can put any commands in your ~/.bnirc file, they will be auto-executed on startup\n"
"check sample.bnirc for an example\n"
"\n"
"run /help to get a list of commands that the client knows\n"
"LOTS MORE that aren't listed work, but are only passed to the server, but i'm not gonna list them\n"
"\n"
"RFC compliance:\n"
"by default the IRC RFC are broken in a few non-dangerous ways to deal with stupid clients, particularly with regards to NOTICE verse PRIVMSG, care has been taken to avoid \"bot-chatter\" when in this mode, if you want you can go to strict compliance with /set strict_rfc on\n"
"\n"
"PREREQS - You need both the libs and headers for the below\n"
"ncurses (ncursesw if you want UTF support)\n"
"python\n"
;
