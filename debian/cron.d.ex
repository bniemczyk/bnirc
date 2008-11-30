#
# Regular cron jobs for the bnirc package
#
0 4	* * *	root	[ -x /usr/bin/bnirc_maintenance ] && /usr/bin/bnirc_maintenance
