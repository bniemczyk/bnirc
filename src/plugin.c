/*
 *
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

plugin_t plugin_head;

INIT_CODE(my_init)
{
	list_init(&plugin_head);
}

void plugin_list(void)
{
	plugin_t *i;

	io_colored_out(USER_GREEN, "%*s %*s %*s  DESCRIPTION\n", 30, "PLUGIN",
		       15, "NAME", 8, "VERSION");
	for (i = (plugin_t *) plugin_head.head.next; i != &plugin_head;
	     list_next(i)) {
		io_colored_out(USER_WHITE, "%*s %*s %*s  %s\n", 30, i->filename,
			       15, i->name, 8, i->version, i->description);
	}
}

static plugin_t *find_plugin(const char *name)
{
	plugin_t *i = (plugin_t *) plugin_head.head.next;

	for (; i != &plugin_head; list_next(i))
		if (!strcmp(i->name, name))
			return i;

	return NULL;
}

void reload_plugin(const char *name)
{
	assert(name);

	plugin_t *p = find_plugin(name);
	if (!p)
		return;

	char *tmp = malloc(strlen(p->loadstring) + 1);
	strcpy(tmp, p->loadstring);
	unload_plugin(p->name);
	load_plugin(tmp);
	free(tmp);
}

void reload_all_plugins(void)
{
	plugin_t local_head;
	list_init(&local_head);

	plugin_t *i;
	plugin_t *tmp;

	for (i = (plugin_t *) plugin_head.head.next; i != &plugin_head;
	     list_next(i)) {
		tmp = malloc(sizeof(plugin_t));
		memcpy(tmp, i, sizeof(plugin_t));
		list_insert(&local_head, tmp);
	}

	for (i = (plugin_t *) local_head.head.next; i != &local_head;
	     list_next(i)) {
		reload_plugin(i->name);
	}
}

void unload_all_plugins(void)
{
	plugin_t *i;

	for (i = (plugin_t *) plugin_head.head.next; i != &plugin_head;) {
		plugin_t *tmp = i;
		list_next(i);
		unload_plugin(tmp->name);
	}
}

static void __attribute__ ((destructor)) fini(void)
{
	unload_all_plugins();
}

static plugin_t *find_plugin_by_filename(const char *name)
{
	plugin_t *i = (plugin_t *) plugin_head.head.next;

	if (bool_option("verbose")) {
		cio_out("find_plugin_by_filename(%s)\n", name);
	}

	for (; i != &plugin_head; list_next(i))
		if (i->filename != NULL && !strcmp(i->filename, name))
			return i;

	return NULL;
}

void register_plugin(plugin_t * p)
{
	if (find_plugin(p->name))
		return;

	list_insert(plugin_head.head.prev, p);
}

static char *find_plugin_file(const char *pname)
{
	size_t len = strlen(pname) + strlen(PLUGIN_DIR) + strlen("lib.la") + 1;
	char *buf = malloc(len);
	sprintf(buf, PLUGIN_DIR "lib%s.la", pname);
	FILE *f = bnirc_fopen(buf, "r");
	char *rv = NULL;

	if (!f) {
		free(buf);
		return NULL;
	}

	while (1) {
		bnirc_getline(&buf, &len, f);

		if (feof(f) || ferror(f))
			break;

		if (strncmp(buf, "dlname='", strlen("dlname='")))
			continue;

		size_t i = strlen("dlname='");
		len = strlen(&buf[i]) - 2;
		rv = malloc(len + 1);
		strncpy(rv, &buf[i], len);
		rv[len] = 0;
		break;
	}

	free(buf);
	fclose(f);
	return rv;
}

int init_plugin(plugin_t * p, const char *cdata, void *handle, int argc,
		char **argv)
{
	assert(p != NULL);

	const char *ci;

	/**********************************************
	 * load dependancies
	 **********************************************/
	if (p->deps != NULL) {
		for (ci = p->deps[0]; ci != NULL; ci++) {
			if (load_plugin(ci))
				return -1;
		}
	}

	p->handle = handle;
	p->loadstring = malloc(strlen(cdata) + 1);
	strcpy(p->loadstring, cdata);

	p->filename = malloc(strlen(argv[0]) + 1);
	strcpy(p->filename, argv[0]);

	if (!p->name || !p->version || !p->description) {
		list_remove(p);
		return -1;
	}

	if (p->io_driver != NULL)
		register_io_driver(p->io_driver);

	if (p->command_list != NULL) {
		register_command_list(p->command_list);
	}

	if (p->server_string_list != NULL) {
		register_server_string_list(p->server_string_list);
	}

	if (p->plugin_init != NULL)
		if (p->plugin_init(argc, argv)) {
			list_remove(p);
			return -1;
		}

	return 0;
}

/**
 * loads a plugin
 *
 * returns 0 on success, or if the plugin is already loaded
 * 1 on error
 **/
int load_plugin(const char *cdata)
{
	const char *i = cdata;
	char **argv;
	int argc;
	char *buf = NULL;
	void *handle;
	plugin_t *p;

	plugin_t *(*pinit) (void);

	while (*i == ' ')
		i++;

	/* build argv/argc */

	argc = 0;
	argv = (char **) NULL;
	make_argv(&argc, &argv, cdata);
	assert(argc != 0);

	char *init_func =
	    malloc(strlen("__do_register_") + strlen(argv[0]) + 1);
	sprintf(init_func, "__do_register_%s", argv[0]);

	if (find_plugin(argv[0])) {
		cio_out("%s is already loaded\n", argv[0]);
		goto load_plugin_out;
	}

	char *libname = find_plugin_file(argv[0]);

	if (!libname)
		goto load_plugin_fail;

	free(argv[0]);
	argv[0] = libname;


	if (argc > 30)
		io_colored_out(USER_RED,
			       "warning: too many arguments, not all will be passed to the plugin!\n");

	buf = malloc(strlen(argv[0]) + strlen(PLUGIN_DIR) + 1);
	strcpy(buf, PLUGIN_DIR);
	strcpy(&buf[strlen(PLUGIN_DIR)], argv[0]);

	handle = bnirc_dlopen(buf, RTLD_NOW | RTLD_GLOBAL);
	if (!handle) {
		io_colored_out(USER_RED, "error: could not load %s\n", buf);
		io_colored_out(USER_RED, "%s\n", dlerror());
		goto load_plugin_fail;
	}

	cio_out("loading %s\n", buf);

	pinit = dlsym(handle, init_func);
	if (!pinit) {
		io_colored_out(USER_RED,
			       "error: %s does not seem to be a plugin\n", buf);
		io_colored_out(USER_RED, "%s could not be found!\n", init_func);
		goto load_plugin_fail;
	}

	p = pinit();
	if (init_plugin(p, cdata, handle, argc, argv))
		goto load_plugin_fail;

      load_plugin_out:
	free_argv(argc, argv);
	free(init_func);
	if (buf)
		free(buf);
	return 0;

      load_plugin_fail:
	io_colored_out(USER_RED, "plugin load failed!\n");
	free_argv(argc, argv);
	free(init_func);
	free(buf);
	return 1;
}

void unload_plugin(const char *name)
{
	plugin_t *p;

	p = find_plugin_by_filename(name);
	if (!p)
		p = find_plugin(name);

	if (!p) {
		io_colored_out(USER_RED, "error: %s is not loaded\n", name);
		return;
	}

	if (p->ref) {
		cio_out("%s is in use, cannot unload\n", p->name);
		return;
	}

	cio_out("unloading %s\n", p->name);

	if (p->plugin_cleanup)
		p->plugin_cleanup();

	if (p->command_list)
		unregister_command_list(p->command_list);

	if (p->server_string_list)
		unregister_server_string_list(p->server_string_list);

	if (p->io_driver)
		unregister_io_driver(p->io_driver);

	list_remove(p);

	if (p->handle != NULL)
		dlclose(p->handle);
}

void plugin_irc_hook(char *cdata)
{
	plugin_t *i;

	for (i = (plugin_t *) plugin_head.head.next; i != &plugin_head;
	     list_next(i)) {
		if (i->irc_hook)
			i->irc_hook(cdata);
	}
}

void plugin_input_hook(char *cdata)
{
	plugin_t *i;

	for (i = (plugin_t *) plugin_head.head.next; i != &plugin_head;
	     list_next(i)) {
		if (i->input_hook)
			i->input_hook(cdata);
	}
}

void plugin_ctcp_in_hook(const char *who, const char *victim, const char *cdata,
			 const char *server)
{
	if (cdata[0] != '\1')
		return;

	char *buf = malloc(strlen(cdata) + 1);
	strncpy(buf, &cdata[1], strlen(cdata) - 2);
	buf[strlen(cdata) - 2] = 0;

	plugin_t *i;
	for (i = (plugin_t *) plugin_head.head.next; i != &plugin_head;
	     list_next(i)) {
		if (i->ctcp_in_hook)
			i->ctcp_in_hook(who, victim, buf, server);
	}

	free(buf);
}

void plugin_privmsg_hook(const char *who, const char *victim, const char *cdata,
			 const char *server)
{
	if (cdata[0] != '\1')
		return;
/* @DISABLED@

	/COMMENT*
	char *buf = malloc(strlen(cdata) + 1);
	strncpy(buf, cdata, strlen(cdata));
	buf[strlen(cdata)] = 0;
	*COMMENT/
	char *buf = malloc(strlen(cdata) + 1);
	strcpy(buf, cdata);

 @DISABLED@ */

	char *buf = copy_string(cdata);

	plugin_t *i;
	for (i = (plugin_t *) plugin_head.head.next; i != &plugin_head;
	     list_next(i)) {
		if (i->privmsg_hook)
			i->privmsg_hook(who, victim, buf, server);
	}

	free(buf);
}
