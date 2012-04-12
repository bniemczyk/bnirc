/**************************************
 * Copyright (C) 2005 Brandon Niemczyk
 *
 * Description:
 *    Basic regular expressions, in the form of regex(flags, regex, input, ...)
 *    where ... are char ** to place group matches in
 *
 *    Example:
 *
 *         char *input = "Name: Brandon Niemczyk";
 *         char *name = NULL;
 *         if(regex(RGX_STOREGROUPS, "Name: (.*)", input, &name)) {
 *            do_something(name); // name eq "Brandon Niemczyk"
 *            free(name);
 *         }
 *
 *    would allocate name, and put "Brandon Niemczyk" into it
 *    regex() returns non-zero on match
 *
 * ChangeLog:
 *
 * License: GPL Version 2
 *
 * Todo:
 *    clean the consumed memory up, this is priority #1
 *
 **************************************/

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "irc.h"

#ifndef STRNDUP_AVAIL
static char *strndup(const char *in, size_t n)
{
	size_t len = strlen(in);
	if (len <= n)
		return strdup(in);

	char *out = malloc(n + 1);
	strncpy(out, in, n);
	out[n] = 0;
	return out;
}
#endif

static int rgx_debug;
static int rgx_case_insens;

#define match_no_case(a, b) ( isalpha(a) && isalpha(b) ? toupper(a) == toupper(b) : 0 )
static int charclass(int c, const char *class)
{
	int i;

	if (c < 1 || c > 255)
		return 0;

	for (i = 0; class[i] != '\0'; i++) {
		if (c == class[i]
		    || (rgx_case_insens && match_no_case(c, class[i])))
			return i + 1;
	}

	return 0;
}

/**********************************************
 * if you edit these, you MUST edit bn.h too
 **********************************************/
 /*
    #define RGX_STOREGROUPS 0x1
  */

/**********************************************
 * token types
 **********************************************/
enum {
	tok_CARROT,		/* ^ */
	tok_BRACKET_IN,		/* [ */
	tok_BRACKET_OUT,	/* ] */
	tok_PAREN_IN,		/* ( */
	tok_PAREN_OUT,		/* ) */
	tok_OR,			/* | */
	tok_CLASS,
	tok_GREEDY_ONE,		/* + */
	tok_GREEDY_ZERO,	/* * */
	tok_ONE_OR_ZERO,	/* ? */
	tok_DOLLAR,		/* $ */
	tok_TOK_VECTOR,
	tok_END_VECTOR
};

typedef struct token {
	int type;
	char *class;
	int flags;
	int tok_vector_count;
	struct token **tok_vectors;
	int *tok_vector_sizes;
	int matched_vector;
	char *match;
} token_t;

static int match_min(token_t * tok, int veclen);

static void free_token_vector(token_t * tok, int count)
{
	int i, j;

	for (i = 0; i < count; i++) {
		if (tok[i].class != NULL)
			free(tok[i].class);
		if (tok[i].tok_vector_count != 0) {
			for (j = 0; j < tok[i].tok_vector_count; j++) {
				free_token_vector(tok[i].
						  tok_vectors
						  [j],
						  tok[i].tok_vector_sizes[j]);
			}
			free(tok[i].tok_vector_sizes);
			free(tok[i].tok_vectors);
		}
		if (tok[i].match != NULL)
			free(tok[i].match);
	}

	free(tok);
}

/**********************************************
 * special classes
 **********************************************/
typedef struct special_class {
	const char *rep;
	const char *class;
} special_class_t;

special_class_t special_classes[] = {
	{"\\s", " 	"},
	{"\\d", "1234567890"},
	{"\\x", "123456789abcdef"},
	{"\\w",
	 "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-"},
	{"\\a",
	 "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"}
};

typedef struct special_token {
	const char *rep;
	int type;
} special_token_t;

/**********************************************
 * string representation of tokens that do not
 * represent a class
 **********************************************/
special_token_t special_tokens[] = {
	{"^", tok_CARROT},
	{"[", tok_BRACKET_IN},
	{"]", tok_BRACKET_OUT},
	{"(", tok_PAREN_IN},
	{")", tok_PAREN_OUT},
	{"|", tok_OR},
	{"+", tok_GREEDY_ONE},
	{"*", tok_GREEDY_ZERO},
	{"?", tok_ONE_OR_ZERO},
	{"$", tok_DOLLAR}
};

/**********************************************
 * token flags
 **********************************************/
#define NEGATE 0x4
#define GREEDY_ONE 0x8
#define GREEDY_ZERO 0x10
#define ONE_OR_ZERO 0x20
#define MATCH_AT_END 0x40
#define MATCH_AT_BEGIN 0x80

/**********************************************
 * lexcal analysis is pretty easy, and should
 * be somewhat self explanatory, lex__() returns
 * a pointer the the _next_ character after the
 * token in the string, or NULL on error
 **********************************************/
static const char *lex__(const char *str, token_t * tok)
{
	int i;

	memset(tok, 0, sizeof(token_t));

	/* check against special classes */
	for (i = 0; i < sizeof(special_classes) / sizeof(special_class_t); i++) {
		if (strncmp
		    (special_classes[i].rep, str,
		     strlen(special_classes[i].rep)) == 0) {
			tok->type = tok_CLASS;
			tok->class =
			    malloc(strlen(special_classes[i].class) + 1);
			strcpy(tok->class, special_classes[i].class);
			return str + strlen(special_classes[i].rep);
		}
	}

	/* check if it's an escaped char */
	if (*str == '\\') {
		str++;
		if (*str == 0)
			return NULL;
		tok->type = tok_CLASS;
		tok->class = malloc(2);
		tok->class[0] = *str;
		tok->class[1] = 0;
		return ++str;
	}

	/* check against non-class tokens */
	for (i = 0; i < sizeof(special_tokens) / sizeof(special_token_t); i++) {
		if (strncmp
		    (special_tokens[i].rep, str,
		     strlen(special_tokens[i].rep)) == 0) {
			tok->type = special_tokens[i].type;
			return str + strlen(special_tokens[i].rep);
		}
	}

	/* check if it's a . */
	if (*str == '.') {
		tok->type = tok_CLASS;
		tok->flags = NEGATE;
		tok->class = malloc(2);
		tok->class[0] = '\n';
		tok->class[1] = 0;
		return ++str;
	}

	/* grab the character */
	if (*str == 0)
		return NULL;
	tok->type = tok_CLASS;
	tok->class = malloc(2);
	tok->class[0] = *str;
	tok->class[1] = 0;
	return ++str;
}

/**********************************************
 * takes an array and a current index, and then
 * tries to reduce the top to less tokens, if
 * it succeeds, it will recursivly try again
 * returns the new index, or -1 on error
 **********************************************/

static int reduce(int i, token_t tokens[])
{
	char *tmp;
	int j, k;

	switch (tokens[i].type) {

	case tok_CARROT:
		if (i <= 0 || tokens[--i].type != tok_BRACKET_IN)
			return -1;
		tokens[i].flags |= NEGATE;
		return reduce(i, tokens);

	case tok_BRACKET_OUT:
		if (i <= 1
		    || tokens[i - 1].type != tok_CLASS
		    || tokens[i - 2].type != tok_BRACKET_IN)
			return -1;
		tokens[i - 2].type = tok_CLASS;
		tokens[i - 2].class = tokens[i - 1].class;
		tokens[i - 2].flags |= tokens[i - 1].flags;
		return reduce(i - 2, tokens);

	case tok_GREEDY_ONE:
		if (i < 1 || tokens[i - 1].type != tok_CLASS)
			return -1;
		tokens[i - 1].flags |= GREEDY_ONE;
		return reduce(i - 1, tokens);

	case tok_GREEDY_ZERO:
		if (i < 1 || tokens[i - 1].type != tok_CLASS)
			return -1;
		tokens[i - 1].flags |= GREEDY_ZERO;
		return reduce(i - 1, tokens);

	case tok_ONE_OR_ZERO:
		if (i < 1
		    || (tokens[i - 1].type != tok_CLASS
			&& tokens[i - 1].type != tok_TOK_VECTOR))
			return -1;
		tokens[i - 1].flags |= ONE_OR_ZERO;
		return reduce(i - 1, tokens);

	case tok_DOLLAR:
		tokens[i - 1].flags |= MATCH_AT_END;
		return reduce(i - 1, tokens);

	case tok_CLASS:
		if (tokens[i - 1].type == tok_CLASS
		    && tokens[i - 2].type == tok_BRACKET_IN) {
			tmp =
			    string_cat(2, tokens[i - 1].class, tokens[i].class);
			free(tokens[i - 1].class);
			free(tokens[i].class);
			tokens[i - 1].class = tmp;
			return reduce(i - 1, tokens);
		}
		break;

	case tok_PAREN_OUT:
	case tok_OR:
		/* find the beginning of the vector */
		for (j = i; j >= 0 && tokens[j].type != tok_PAREN_IN; j--);	/* do nothing */
		if (tokens[j].type != tok_PAREN_IN)
			return -1;
		k = (i - j) - 1;
		tokens[j].tok_vectors =
		    realloc(tokens[j].tok_vectors,
			    sizeof(token_t *) *
			    (tokens[j].tok_vector_count + 1));
		tokens[j].tok_vector_sizes =
		    realloc(tokens[j].tok_vector_sizes,
			    (tokens[j].tok_vector_count + 1) * sizeof(int));
		tokens[j].tok_vectors[tokens[j].
				      tok_vector_count] =
		    malloc(sizeof(token_t) * k);
		memcpy(tokens[j].
		       tok_vectors[tokens[j].
				   tok_vector_count],
		       &tokens[j + 1], sizeof(token_t) * k);
		tokens[j].tok_vector_sizes[tokens[j].tok_vector_count]
		    = k;
		tokens[j].tok_vector_count++;

		if (tokens[i].type == tok_PAREN_OUT) {
			tokens[j].type = tok_TOK_VECTOR;
		}

		return reduce(j, tokens);
	}

	return i;
}

static int check_regex_valid(token_t * regex, int size)
{
	int i;
	int j;

	for (i = 0; i < size; i++) {
		if (regex[i].type == tok_CLASS)
			continue;
		if (regex[i].type == tok_TOK_VECTOR) {
			for (j = 0; j < regex[i].tok_vector_count; j++) {
				if (!check_regex_valid
				    (regex[i].
				     tok_vectors[j],
				     regex[i].tok_vector_sizes[j]))
					return 0;
			}
			continue;
		}

		printf("bad token type [%d] token #%d\n", regex[i].type, i);
		return 0;
	}

	return 1;
}

static int dump_regex(token_t * regex, int size)
{
	int i;
	int j;
	int k;
	static int tabs = 0;

	for (i = 0; i < size; i++) {
		for (j = 0; j < tabs; j++) {
			printf("\t");
		}

		if (regex[i].type == tok_CLASS) {
			printf
			    ("CLASS(%d) [%s] flags: 0x%x\n",
			     match_min(&regex[i], 1),
			     regex[i].class, regex[i].flags);
			continue;
		}
		if (regex[i].type == tok_TOK_VECTOR) {
			printf("VECTOR(%d) flags: 0x%x\n",
			       match_min(&regex[i], 1), regex[i].flags);
			tabs++;
			for (j = 0; j < regex[i].tok_vector_count; j++) {
				if (j) {
					for (k = 0; k < tabs - 1; k++)
						printf("\t");
					printf("OR\n");
				}

				if (!check_regex_valid
				    (regex[i].
				     tok_vectors[j],
				     regex[i].tok_vector_sizes[j]))
					return 0;
				dump_regex(regex[i].
					   tok_vectors[j],
					   regex[i].tok_vector_sizes[j]);
			}
			tabs--;
			for (j = 0; j < tabs; j++) {
				printf("\t");
			}
			printf("END VECTOR\n");
			continue;
		}

		printf("bad token type [%d] token #%d\n", regex[i].type, i);
		return 0;
	}


	return 1;
}

/**********************************************
 * now that we have a lex__() and reduce()
 * function, compiling our regex is easy
 **********************************************/
void *compile_regex(const char *regex, int *size)
{
	token_t *tok = NULL;
	int alloced = 0;
	int i = 0;

	assert(size != NULL);

	while (1) {
		if (i <= alloced) {
			alloced = (i + 1) << 1;
			tok = realloc(tok, alloced * sizeof(token_t));
			if (tok == NULL)
				return NULL;
		}
		regex = lex__(regex, &tok[i]);
		i = reduce(i, tok);
		if (i == -1)
			return NULL;

		i++;
		if (*regex == 0)
			break;
	}

	/**********************************************
	 * the reduce function isn't all that picky
	 * so we need to check that the regex is really
	 * valid, if it is, it will have only
	 * TOK_VECTOR and CLASS tokens
	 **********************************************/
	if (!check_regex_valid(tok, i))
		return NULL;

	*size = i;
	return tok;
}

/**********************************************
 * while matching we need to include a max
 * field so that we can --max if and try again
 * if the rest of the regex does not match
 **********************************************/

static int match_class(const char *data, token_t * tok, int max)
{
	int i;

	for (i = 0; i < max; i++) {
		if ((!charclass(data[i], tok->class)
		     && (tok->flags & NEGATE) == 0)
		    || (charclass(data[i], tok->class)
			&& (tok->flags & NEGATE) != 0)
		    )
			break;
	}

	return i;
}

/**************************************
 * this gives the minimum size match
 * allowed by a vector of instructions
 **************************************/
static int match_min(token_t * tok, int veclen)
{
	int i, j, min = 0, vmin, vmin_tmp;
	for (i = 0; i < veclen; i++) {
		if (tok[i].type == tok_CLASS) {
			min +=
			    (tok[i].
			     flags & (GREEDY_ZERO | ONE_OR_ZERO)) ? 0 : 1;
		} else {	// it's a vector
			if (tok[i].flags & (GREEDY_ZERO | ONE_OR_ZERO))
				continue;
			vmin =
			    match_min(tok[i].tok_vectors[0],
				      tok[i].tok_vector_sizes[0]);
			for (j = 1; j < tok[i].tok_vector_count; j++) {
				vmin_tmp =
				    match_min(tok[i].
					      tok_vectors
					      [j], tok[i].tok_vector_sizes[j]);
				vmin = vmin_tmp < vmin ? vmin_tmp : vmin;
			}
			min += vmin;
		}
	}

	return min;
}

/**********************************************
 * returns -1 on error, or the amount of times
 * matched otherwise
 **********************************************/
static int match_vector(const char *data, token_t * tok, int max, int veclen)
{
	int tmp, mymax, mymin, me, rest = 0, i;

	if (tok->flags & GREEDY_ZERO
	    || tok->flags & GREEDY_ONE || tok->type == tok_TOK_VECTOR)
		mymax = max;
	else
		mymax = 1;

	mymin = match_min(tok, 1);

	if (mymax < mymin)
		return -1;

	while (1) {
		if (tok->type == tok_CLASS)
			me = match_class(data, tok, mymax);
		else {
			me = -1;
			for (i = 0; i < tok->tok_vector_count; i++) {
				tmp =
				    match_vector(data,
						 tok->
						 tok_vectors
						 [i], mymax,
						 tok->tok_vector_sizes[i]);
				if (tmp > me)
					me = tmp;
			}
		}

		if (me < mymin) {
			me = -1;
			break;
		}

		rest =
		    (veclen > 1) ? match_vector(data + me,
						&tok[1],
						max - me, veclen - 1) : 0;

		if (rest >= 0) {
			if (rgx_debug && tok->type == tok_CLASS) {
				printf
				    ("match found for token:"
				     "class = [%s] : %d, match = [%s]\n",
				     tok->class, tok->flags, strndup(data, me)
				    );
			} else if (rgx_debug) {
				printf
				    ("match found for vector: [%s]\n",
				     strndup(data, me));
			}
			if (rgx_debug)
				printf("sizes: me: %d, rest: %d\n", me, rest);
			break;
		}
		// mymax = me - 1;
		mymax--;
		if (mymax < mymin) {
			me = -1;
			break;
		}
	}

	if (tok->type == tok_TOK_VECTOR) {
		if (me <= 0) {
			tok->match = strdup("");
		} else {
			tok->match = strndup(data, me);
		}
	}

	if (me == -1 && mymin == 0)
		return veclen > 1 ? match_vector(data,
						 &tok[1], max, veclen - 1) : 0;

	return me + rest;
}

typedef struct {
	list_t header;
	char *match;
} lgroup_t;

lgroup_t lgroups = { {(list_t *) & lgroups, (list_t *) & lgroups}, NULL };

/* pulls the grouped matches out of a regex */
static void vpull_groups(token_t * tok, int veclen, va_list args)
{
	int i;
	char **tmp;

	for (i = 0; i < veclen; i++) {
		if (tok[i].type != tok_TOK_VECTOR)
			continue;
		tmp = va_arg(args, char **);
		*tmp = malloc(strlen(tok[i].match) + 1);
		strcpy(*tmp, tok[i].match);
		vpull_groups(tok[i].
			     tok_vectors[tok[i].
					 matched_vector],
			     tok[i].tok_vector_sizes[tok[i].
						     matched_vector], args);
	}
}

/* pulls the grouped matches out of a regex */
static void pull_groups(token_t * tok, int veclen)
{
	int i;
	lgroup_t *tmp;

	for (i = 0; i < veclen; i++) {
		if (tok[i].type != tok_TOK_VECTOR)
			continue;
		tmp = malloc(sizeof(lgroup_t));
		tmp->match = strdup(tok[i].match);
		push(&lgroups, tmp);
		pull_groups(tok[i].
			    tok_vectors[tok[i].
					matched_vector],
			    tok[i].tok_vector_sizes[tok[i].matched_vector]);
	}
}

static void show_groups(void)
{
	lgroup_t *tmp = (lgroup_t *) lgroups.header.next;
	printf("FOUND GROUPS:\n");
	while (tmp != &lgroups) {
		printf("group: (%s)\n", tmp->match);
		list_next(tmp);
	}
	printf("\n");
}

char *regex_next_group()
{
	if (list_empty(&lgroups))
		return NULL;

	lgroup_t *tmp = (lgroup_t *) bottom(&lgroups);
	char *r = strdup(tmp->match);
	pull(&lgroups);
	free(tmp->match);
	free(tmp);
	return r;
}

int vregex(int flags, const char *regex, const char *data, va_list args)
{
	token_t *crgx;
	int len;
	int test;
	int ret = 0;

	crgx = compile_regex(regex, &len);
	if (crgx == NULL)
		goto out;

	if (flags & RGX_DUMP) {
		dump_regex(crgx, len);
	}

	if (flags & RGX_DEBUG_MATCH)
		rgx_debug = 1;

	if (flags & RGX_IGNORE_CASE)
		rgx_case_insens = 1;
	else
		rgx_case_insens = 0;

	test = match_vector(data, crgx, strlen(data), len);
	rgx_debug = 0;

	if (test < 0)
		goto out;

	list_init(&lgroups);
	pull_groups(crgx, len);

	if (flags & RGX_SHOWGROUPS) {
		show_groups();
	}


	if (flags & RGX_STOREGROUPS) {
		vpull_groups(crgx, len, args);
	}

	free_token_vector(crgx, len);

	ret = 1;
      out:
	return ret;
}

int regex(int flags, const char *regex, const char *data, ...)
{
	va_list args;
	int rv;

	va_start(args, data);
	rv = vregex(flags, regex, data, args);
	va_end(args);

	return rv;
}

int regex_search(const char **start, int flags,
		 const char *regex, const char *data, ...)
{
	va_list args;
	int rv = 0;

	va_start(args, data);
	while (!rv && *data != 0) {
		rv = vregex(flags, regex, data, args);
		data++;
	}
	*start = --data;
	va_end(args);

	return rv;
}
