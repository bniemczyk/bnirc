%
% Copyright (C) Brandon Niemczyk 2004
%
% DESCRIPTION:
%
% CHANGELOG:
%
%

%pragma once

define	botsay		( argc, argv )
{
	if(bool_option("strict_rfc")) {
		irc_putstring("NOTICE " + argv[2] + " :" + argv[3] + "\n");
	} else {
		irc_putstring("PRIVMSG " + argv[2] + " :" + argv[3] + "\n");
	}
}
register_command("botsay", "botsay <who> <msg>", 3, 3, "botsay", "if strict_rfc is on then will NOTICE, otherwise the same as /msg");

define	festival	( local_nick, channel, string )
{
	variable nick = get_current_nick();
	
	variable sys_string;

	if(bool_option(strlow(local_nick) + "_festival") or bool_option(strlow(channel) + "_festival")) {
		(string, ) = strreplace(string, "\\", "\\\\", strlen(string));
		sys_string = "echo \"" + local_nick + " says " + string + "\"|" + string_option("festival_command") + " &>/dev/null";
		if(bool_option("verbose"))
			putstring(sys_string);
		() = system(sys_string);
	} else if(bool_option("speak_all") or ((string_match(string, nick, 1) or strcmp(strlow(channel), strlow(nick)) == 0) and bool_option("address_speak"))) {
		(string, ) = strreplace(string, "\\", "\\\\", strlen(string));
		sys_string = "echo \"" + local_nick + " says " + string + "\"|" + string_option("festival_command") + " &>/dev/null";
		if(bool_option("verbose"))
			putstring(sys_string);
		() = system(sys_string);
	}
}

define	speak_hook	( argc, argv )
{
	set_option(strlow(argv[1]) + "_festival", "on");
}
register_command("speak", "speak <nick | channel>", 2, 2, "speak_hook", 
		"use festival to \"say\" anything spoken in or by the channel/user specified");

define nospeak_hook	( argc, argv )
{
	set_option(strlow(argv[1]) + "_festival", "off");
}
register_command("nospeak", "nospeak <nick | channel>", 2, 2, "nospeak_hook", 
		"turn off /speak for nick/channel specified");

% some hooks
define privmsg_hook	( argc, argv )
{
	variable n = grab_nick(argv[0]);
	nick_privmsg(n, argv[2], argv[3]);
	festival(n, argv[2], argv[3]);
}
register_server_string("privmsg", 1, 4, "privmsg_hook");
