/*
 * Copyright (C) 2004 Brandon Niemczyk
 * 
 * DESCRIPTION:
 *
 * CHANGELOG:
 * 
 * LICENSE: GPL Version 2
 *
 * TODO:
 *
 */

#ifndef NDEBUG
#ifndef __cplusplus
extern int printf(const char *, ...);
#define DPRINT printf
#else
extern "C" int printf(const char *, ...);
#define DPRINT ::printf
#endif
#else
#define DPRINT(x,...)
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#include "plugin.h"

typedef struct dcc_struct {
	hash_entry_t header;
	const char *filename;
	void *file_map;
	unsigned file_len;
	unsigned read;
	FILE *file;
	sem_t lock;

	user_t *user;

	struct sockaddr_in address;
	int port;

	union {
		int bytes_recvd;
		int bytes_sent;
	};

	irc_poll_t poll;

	int type;
#define DCC_SEND 1
#define DCC_RECV 2
#define DCC_CHAT 3

	int state;
#define WAITING 1
#define SENDING 2
#define RECIEVING 3
#define CHAT 4

	unsigned packet_size;

} dcc_t;

hash_t my_dccs;

INIT_CODE(init_dcc_hash)
{
	hash_init(my_dccs);
}

static void incoming_dcc(void *dcc)
{
	/***************************************************************
	 * only handles SEND recieving right now
	 ***************************************************************/
	dcc_t *d = dcc;
	int tmp;
	if (d->type == DCC_RECV) {
		void *buf = malloc(d->packet_size);
		tmp = recv(d->poll.fd, buf, d->packet_size, 0);
		d->read += tmp;
		send(d->poll.fd, &(d->read), sizeof(unsigned), 0);
		fwrite(buf, tmp, 1, d->file);
		free(buf);
	}
}

static void error_dcc(void *dcc)
{
}

static void eof_dcc(void *dcc)
{
	dcc_t *d = dcc;
	if (d->type == DCC_RECV) {
		fclose(d->file);
	}
}

static dcc_t *dcc_construct(const char *name, const char *port, const char *ip,
			    unsigned packet_size);

static int accept_dcc ( dcc_t *dcc )
{
	unsigned addr;
	memcpy(&addr, &(dcc->address.sin_addr), sizeof(unsigned));
	io_colored_out(USER_WHITE, "accepting dcc offer, connecting to %d.%d.%d.%d:%d (0x%x)\n",
			(addr & 0xff000000) >> 6,
			(addr & 0xff0000) >> 4,
			(addr & 0xff00) >> 2,
			(addr & 0xff),
			(int)ntohs(dcc->address.sin_port),
			addr
			);
	dcc->file = fopen(dcc->filename, "w");
	if(dcc->file) {
		return 0;
		io_colored_out(USER_RED, "could not open %s for writing", dcc->filename);
	}
	int c = connect(dcc->poll.fd, (struct sockaddr *) &(dcc->address), sizeof(struct sockaddr_in));
	if(c) {
		io_colored_out(USER_RED, "Error Connecting!\n");
		return 0;
	}
	send(dcc->poll.fd, &(dcc->packet_size), sizeof(unsigned), 0);
	register_poll(&(dcc->poll));
	dcc->state = RECIEVING;
	return 1;
}

static int	ctcp_hook	( const char *who, const char *victim, const char *cdata, const char *server )
{
	user_t *me = find_user(victim, server);
	dcc_t *d;


	if(!(me->flags & USER_IS_SELF))
		return -1;

	char *port, *ip, *filename;
	if(bool_option("verbose")) {
		cio_out("checking if [%s] is a DCC OFFER\n", cdata);
	}
	if(regex(RGX_STOREGROUPS, "DCC SEND (.*) (\\d+) (\\d+)",
				cdata, &filename, &ip, &port)) {
		// this is an offer
		io_colored_out(USER_RED, "DCC SEND OFFER: %s\n", cdata);
		d = dcc_construct(filename, port, ip, 1024);
		d->filename = strdup(filename);
		d->type = DCC_RECV;
		free(filename);
		free(port);
		free(ip);
		if(bool_option("auto_accept_dcc")) {
			if(!accept_dcc(d))
				return 1;
			return 0;
		} else {
			d->state = WAITING;
			return 0;
		}
	}

	return 1;
}

static dcc_t *dcc_construct(const char *name, const char *port, const char *ip,
			    unsigned packet_size)
{
	unsigned _ip = atol(ip);
	dcc_t *dcc = malloc(sizeof(dcc_t));
	dcc->address.sin_family = AF_INET;
	dcc->address.sin_port = htons(atoi(port));
	if(bool_option("verbose")) {
		cio_out("dcc IP = 0x%x [%s]\n", _ip, ip);
	}
	memcpy(&(dcc->address.sin_addr), &_ip, 4);
	dcc->poll.fd = socket(PF_INET, SOCK_STREAM, 0);
	dcc->poll.data = dcc;
	dcc->poll.in = fdopen(dcc->poll.fd, "r");
	dcc->poll.out = fdopen(dcc->poll.fd, "w");
	dcc->poll.callback = incoming_dcc;
	dcc->poll.error_callback = error_dcc;
	dcc->poll.eof_callback = eof_dcc;
	dcc->poll.data = dcc;
	dcc->packet_size = packet_size ? packet_size : 1024;
	dcc->file_map = NULL;
	dcc->file_len = 0;
	dcc->read = 0;
	dcc->file = NULL;
	sem_init(&dcc->lock, 0, 1);
	hash_insert(my_dccs, dcc, name);

	return dcc;

	return 0;
}

static int init(int argc, char *argv[])
{
	cio_out
	    ("dcc is barely implemented as of now... it's a shitty protocol and brandon is being stubborn/lazy\n");
	cio_out("and it doesn't work\n");
	return 0;
}


static plugin_t plugin = {
	.name = "dcc",
	.description = "provides /dcc",
	.version = "0.0.1",
	.ctcp_in_hook = ctcp_hook,
	.plugin_init = init
};

REGISTER_PLUGIN(plugin, bnirc_dcc)
