/*
 * Copyright (C) 2004 Brandon Niemczyk
 * 
 * DESCRIPTION:
 * 	functions that don't really belong anywhere else
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "irc.h"

int _global_lock_init_needed = 1;
int irc_shutdown = 0;

/* {}|^ are considered to be the lower case equivelants of []\= in IRC */
#define lc(x) ( x >= 'A' && x <= 'Z' ? x + ('a' - 'A') : \
		x == '[' ? '{' : \
		x == ']' ? '}' : \
		x == '\\' ? '|' : \
		x == '-' ? '^' : \
		x )

#define WSPACE "\n\r\f\t\v "

static const char *dirs[] = {
	DATADIR,
	LIBDIR,
	NULL
};


/**********************************************
 * i set HAVE_BACKTRACE in my configure.ac
 **********************************************/
#ifdef HAVE_BACKTRACE

#ifdef backtrace
#undef backtrace
#endif
void __real_backtrace(void)
{
	void *bt[128];
	int bt_size;
	char **bt_syms;
	int i;

	bt_size = backtrace(bt, 128);
	bt_syms = backtrace_symbols(bt, bt_size);
	io_colored_out(USER_WHITE, "BACKTRACE -----------\n");
	for (i = 1; i < bt_size; i++) {
		cio_out("%s\n", bt_syms[i]);
	}
	io_colored_out(USER_WHITE, "---------------------\n");
}
#endif

/**********************************************
 * malloc wrapper that makes sure everything
 * gets set to 0
 **********************************************/
#ifdef malloc
#undef malloc
#endif
void *bnirc_zmalloc(ssize_t s)
{
	void *r = malloc(s);
	memset(r, 0, s);
	return r;
}

#define malloc(x) bnirc_zmalloc(x)

/*
 * looks in all the approp directories
 */
char *bnirc_find_file(const char *name)
{
	assert(name);

	char *buf = malloc(412);
	size_t i;
	FILE *f;

	if (bool_option("verbose"))
		cio_out("checking for %s\n", name);

	if ((f = fopen(name, "r")) != NULL) {
		fclose(f);
		return copy_string(name);
	}


	strncpy(buf, getenv("HOME"), 200);
	strcat(buf, "/.bnIRC/");
	strncat(buf, name, 200);

	if (bool_option("verbose"))
		cio_out("checking for %s\n", buf);

	if ((f = fopen(buf, "r")) != NULL) {
		fclose(f);
		return buf;
	}

	for (i = 0; f == NULL && dirs[i] != NULL; i++) {
		strncpy(buf, dirs[i], 200);
		strncat(buf, name, 200);
		if (bool_option("verbose"))
			cio_out("checking for %s\n", buf);
		f = fopen(buf, "r");
		if (f) {
			fclose(f);
			return buf;
		}
	}

	return NULL;
}

void **array_sort (int (*comparer)(void *, void *), void **array, int array_len)
{
    void *head = *array;

    /* this requires gcc */
    int lo_filter(void *a, void *b) 
    {
        return comparer(head, array) < 1 ? 1 : 0;
    }

    int hi_filter(void *a, void *b)
    {
        return comparer(head, array) >= 1 ? 1 : 0;
    }

    /* special case */
    if(array_len <= 1) 
    {
        void **rv = malloc(sizeof(void *) * array_len);
        memcpy(rv, array, array_len * sizeof(void *));
        return rv;
    }

    void **rv = malloc(sizeof(void *) * array_len);
    void **p = rv;

    int fcount = 0;
    void **lo = array_filter(lo_filter, array, &fcount);
    lo = array_sort(comparer, lo, fcount);
    memcpy(p, lo, fcount * sizeof(void *));
    p += fcount;

    *p++ = head;

    void **hi = array_filter(hi_filter, array, &fcount);
    hi = array_sort(comparer, hi, fcount);
    memcpy(p, hi, fcount * sizeof(void *));

    return rv;
}

void **array_filter(int (*filter) (void *), void **array, int *array_len)
{
    int count = 0;
    int i;

    for(i = 0; i < *array_len; i++) 
    {
        if(filter(array[i]))
            count += 1;
    }

    void **rv = malloc(sizeof(void *) * count);
    void **item = rv;

    for(i = 0; i < *array_len; i++)
    {
        if(filter(array[i]))
        {
            *item = array[i];
            item++;
        }
    }

    *array_len = count;
    return rv;
}

/*
 * looks in all the approp directories
 */
FILE *bnirc_fopen(const char *name, const char *mode)
{
	assert(name);
	assert(mode);

	char buf[412];
	size_t i;
	FILE *f;

	if (bool_option("verbose"))
		cio_out("trying to open %s\n", name);

	if ((f = fopen(name, mode)) != NULL)
		return f;


	strncpy(buf, getenv("HOME"), 200);
	strcat(buf, "/.bnIRC/");
	strncat(buf, name, 200);

	if (bool_option("verbose"))
		cio_out("trying to open %s\n", buf);

	if ((f = fopen(buf, mode)) != NULL)
		return f;

	for (i = 0; f == NULL && dirs[i] != NULL; i++) {
		strncpy(buf, dirs[i], 200);
		strncat(buf, name, 200);
		if (bool_option("verbose"))
			cio_out("trying to open %s\n", buf);
		f = fopen(buf, mode);
	}

	return f;
}

/*
 * looks in all the approp directories
 */
void *bnirc_dlopen(const char *name, int mode)
{
	assert(name);

	char buf[412];
	size_t i;
	void *f;

	if (bool_option("verbose"))
		cio_out("trying to open %s\n", name);

	if ((f = dlopen(name, mode)) != NULL)
		return f;


	strncpy(buf, getenv("HOME"), 200);
	strcat(buf, "/.bnIRC/");
	strncat(buf, name, 200);

	if (bool_option("verbose"))
		cio_out("trying to open %s\n", buf);

	if ((f = dlopen(buf, mode)) != NULL)
		return f;

	for (i = 0; f == NULL && dirs[i] != NULL; i++) {
		strncpy(buf, dirs[i], 200);
		strncat(buf, name, 200);
		if (bool_option("verbose"))
			cio_out("trying to open %s\n", buf);
		f = dlopen(buf, mode);
	}

	return f;
}

/*
 * case insensitive strcmp and strncmp
 *
 * FIXME:
 *  are these needed? what about
 *  strcasecmp and strncasecmp?
 */
int strcmp_nc(const char *one, const char *two)
{
	assert(one);
	assert(two);

	char *lc1 = string_lc(one);
	char *lc2 = string_lc(two);
	int rv = strcmp(lc1, lc2);
	free(lc1);
	free(lc2);
	return rv;
}

int strncmp_nc(const char *one, const char *two, size_t n)
{
	assert(one);
	assert(two);

	char *lc1 = string_lc(one);
	char *lc2 = string_lc(two);
	int rv = strncmp(lc1, lc2, n);
	free(lc1);
	free(lc2);
	return rv;
}

/*
 * case insensitive version of strstr
 */
char *strstr_nc(const char *haystack, const char *needle)
{
	assert(haystack);
	assert(needle);

	if (haystack == NULL || needle == NULL)
		return NULL;

	size_t goal = strlen(needle);
	size_t found = 0;
	const char *i;

	for (i = haystack; *i != 0; i++) {
		if (lc(needle[found]) == lc(*i))
			found++;
		else
			found = 0;

		if (found == goal)
			return (char *) (i - (goal - 1));
	}

	return NULL;
}

static int word_count(const char *str)
{
	int count = 0;
	const char *i;

	i = str;
	while (*i) {
		if (isspace(*i)) {
			i++;
			continue;
		}

		count++;

		while (!isspace(*i) && (*i) != 0)
			i++;
	}

	return count;
}

void make_argv(int *argc, char ***argv, const char *str)
{
	const char *i;
	size_t size;
	int tmp;

	/* figure out argc */
	*argc = word_count(str);

	/* initialize argv */
	*argv = malloc(sizeof(char *) * (*argc + 1));
	(*argv)[*argc] = NULL;

	/* populate argv */
	i = str;
	tmp = 0;
	while (*i) {
		if (isspace(*i)) {
			i++;
			continue;
		}

		size = strcspn(i, WSPACE);
		(*argv)[tmp] = malloc(size + 1);
		strncpy((*argv)[tmp], i, size);
		(*argv)[tmp][size] = 0;
		i += size;
		tmp++;
	}
}

void make_max_argv(int *argc, char ***argv, const char *str, size_t n)
{
	const char *i;
	size_t size;
	int tmp;
	int add_extra;

	/* figure out argc */
	*argc = word_count(str);
	if (*argc >= n) {
		*argc = n;
	}

	if (bool_option("verbose"))
		cio_out("argc = %d\n", *argc);

	/* initialize argv */
	*argv = malloc(sizeof(char *) * (*argc + 1));
	(*argv)[*argc] = NULL;

	/* populate argv */
	i = str;
	tmp = 0;
	while (*i && tmp < n - 1) {
		add_extra = 0;

		if (isspace(*i)) {
			i++;
			continue;
		}

		if(*i == '"') {
			i++;
			size = strcspn(i, "\"");
			add_extra = 1;
		} else if (*i == '\'') {
			i++;
			size = strcspn(i, "'");
			add_extra = 1;
		} else {
			size = strcspn(i, WSPACE);
		}

		(*argv)[tmp] = malloc(size + 1);
		strncpy((*argv)[tmp], i, size);
		(*argv)[tmp][size] = 0;

		if (bool_option("verbose"))
			cio_out("argv[%d] = \"%s\"\n", tmp, (*argv)[tmp]);

		i += size + add_extra;
		tmp++;
	}

	if (*argc != n)
		return;		/* we are done here */

	/* get the final entry to argv */
	while (*i) {
		assert(tmp == *argc - 1);
		if (!isspace(*i))
			break;
		i++;
	}

	(*argv)[*argc - 1] = malloc(strlen(i) + 1);
	strcpy((*argv)[*argc - 1], i);

	if (bool_option("verbose"))
		cio_out("argv[%d] = \"%s\"\n", *argc - 1, (*argv)[*argc - 1]);
}

void free_argv(int argc, char **argv)
{
	size_t i;

	for (i = 0; i < argc; i++)
		free(argv[i]);

	free(argv);
}

/*
 * remove a character at position i
 * from str
 */
void remove_char_from_str(char *str, size_t i)
{
	size_t len = strlen(str);
	for (; i < len; i++)
		str[i] = str[i + 1];
}

void add_char_to_str(char *str, size_t i, char c)
{
	size_t len = strlen(str);
	size_t j;
	for (j = len; j > i; j--)
		str[j] = str[j - 1];

	str[j] = c;
	str[len + 1] = 0;
}

char *timestamp(void)
{
	time_t now;
	int i;
	char *buf;
	char *str;
	int num_count = 0;

	time(&now);
	str = asctime(localtime(&now));

	for (i = 0; i < strlen(str); i++) {
		if (str[i] <= '9' && str[i] >= '1') {
			num_count++;
		}
		if (num_count && str[i] == ' ')
			break;
	}
	i++;

	buf = malloc(9);
	strncpy(buf, &str[i], 8);
	buf[8] = 0;

	return buf;
}

/*
 * replace ' ' with %20
 * replace '"' with %22
 *
 * Changes:
 *   replacing ' ' with + instead of %20 now
 */
char *urlize(const char *str)
{
	assert(str);

	char *url;
	size_t len = strlen(str);
	size_t i;
	size_t j;

	for (i = 0; str[i] != 0; i++) {
		if (str[i] == ' ' || str[i] == '"')
			len += 2;
	}

	url = malloc(len + 1);

	for (i = 0, j = 0; str[i] != 0; i++, j++) {
		switch (str[i]) {
		case ' ':
			url[j] = '+';
			break;
		case '"':
			url[j++] = '%';
			url[j++] = '2';
			url[j] = '2';
			break;
		case ':':
			url[j++] = '%';
			url[j++] = '3';
			url[j] = 'A';
			break;
		default:
			url[j] = str[i];
		}
	}

	url[j] = 0;

	return url;
}

char *grab_nick(const char *server_text)
{
	size_t len = 0;

	while (server_text[len + 1] != '!' && server_text[len + 1] != 0
	       && server_text[len + 1] != ' ') {
		len++;
	}

	char *nick = malloc(len + 1);
	strncpy(nick, &server_text[1], len);
	nick[len] = 0;

	if (bool_option("verbose"))
		cio_out("parsed nick [%s] from [%s]\n", nick, server_text);

	return nick;
}

void fgoogle(const char *str)
{
	assert(str);

	const char *channel = natural_channel_name();

	if (!channel) {
		io_out("don't use fgoogle till your in a channel!\n");
		return;
	}
#define fgoogle "http://fuckinggoogleit.com/search?query="
#define FGOOGLE_SIZE (sizeof(fgoogle) - 1)
	char *search = urlize(str);
	assert(search);
	size_t len = FGOOGLE_SIZE;
	len += strlen(search);
	char *url = malloc(len + 1);
	strcpy(url, fgoogle);
	strcpy(&url[FGOOGLE_SIZE], search);

	irc_say(url);

	free(search);
	free(url);
#undef fgoogle
#undef FGOOGLE_SIZE
}

char *copy_string(const char *str)
{
	assert(str != NULL);

	char *rv = malloc(strlen(str) + 1);
	strcpy(rv, str);
	return rv;
}

char *get_username(void)
{
	char *rv;
#ifdef HAVE_PWD_H
	struct passwd *pw = getpwuid(getuid());
	assert(pw != NULL);
	rv = strdup(pw->pw_name);
	// free(pw);
	return rv;
#else
	return copy_string("bnircuser");
#endif
}

void google(const char *str)
{
	assert(str);

	const char *channel = natural_channel_name();

	if (!channel) {
		io_out("don't use fgoogle till your in a channel!\n");
		return;
	}
#define google "http://www.google.com/search?query="
#define GOOGLE_SIZE (sizeof(google) - 1)
	char *search = urlize(str);
	assert(search);
	size_t len = GOOGLE_SIZE;
	len += strlen(search);
	char *url = malloc(len + 1);
	strcpy(url, google);
	strcpy(&url[GOOGLE_SIZE], search);

	irc_say(url);

	free(search);
	free(url);
#undef google
#undef GOOGLE_SIZE
}

/*
 * usleep that does not use SIGALARM
 */
unsigned int bnirc_usleep(unsigned usec)
{
	struct timeval tv = {
		.tv_sec = usec / 1000000,
		.tv_usec = usec % 1000000
	};

	select(0, NULL, NULL, NULL, &tv);
	return 0;
}

/*
 * replacement for GNU's getline() extension because it doesn't exist
 * on FBSD or Cygwin.  Also GNU's doesn't seem to interact to well with sockets
 */
ssize_t bnirc_getline(char **lineptr, size_t * n, FILE * stream)
{
	assert(stream != NULL);
	assert(n != NULL);

	if (stream == NULL || n == NULL)
		return -1;

	int buf;
	size_t len = 0;

	if (*n == 0 || *lineptr == NULL) {
		*n = 12;
		*lineptr = malloc(12);
	}

	while ((buf = fgetc(stream))) {
		if (buf == EOF) {
			return -1;
		}

		if (*n <= len + 2) {
			*n <<= 1;
			*lineptr = realloc(*lineptr, *n);
			assert(*lineptr != NULL);
		}

		(*lineptr)[len++] = (char)buf;

		if (buf == '\n')
			break;
	}

	(*lineptr)[len] = 0;

	return len - 1;
}

/*
 * replaces first instance of needle in haystack with replacement
 */
char *string_replace(const char *haystack, const char *needle,
		     const char *replacement)
{
	assert(haystack);
	assert(needle);
	assert(replacement);

	const char *found = strstr(haystack, needle);
	if (!found)
		return copy_string(haystack);

	size_t len = strlen(haystack) + (strlen(replacement) - strlen(needle));
	char *rv = malloc(len + 1);

	size_t s_index = ((size_t) found - (size_t) haystack) / sizeof(char);

	strncpy(rv, haystack, s_index);
	strncpy(&rv[s_index], replacement, strlen(replacement));
	(&rv[s_index])[strlen(replacement)] = 0;	// yuck!!!

	if (s_index + strlen(needle) < strlen(haystack))
		strcpy(&rv[s_index + strlen(replacement)],
		       &haystack[s_index + strlen(needle)]);

	return rv;
}

/*
 * concatenates multiple strings together with a ' ' between
 * this should be free()'d
 */
char *string_cat(size_t count, ...)
{
	va_list ap;
	size_t i;
	char *tmp;
	char *rv = NULL;
	size_t len = 0;

	va_start(ap, count);
	for (i = 0; i < count; i++) {
		tmp = va_arg(ap, char *);
		rv = realloc(rv, strlen(tmp) + 1 + len);
		strcpy(&rv[len], tmp);
		len += strlen(tmp) + 1;
		rv[len - 1] = ' ';
	}

	rv[len - 1] = 0;
	return rv;
}

/*
 * make a string lowercase
 */
char *string_lc(const char *str)
{
	assert(str != NULL);

	int i;

	char *rv = malloc(strlen(str) + 1);

	for (i = 0; i < strlen(str); i++) {
		if ('A' <= str[i] && str[i] <= 'Z')
			rv[i] = str[i] + ('a' - 'A');
		else
			rv[i] = str[i];
	}

	rv[i] = 0;
	return rv;
}
