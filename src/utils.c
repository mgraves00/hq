/* $Id$ */
/*
 * Copyright (c) 2024 Michael Graves
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "config.h"

#if HAVE_SYS_QUEUE
#include <sys/queue.h>
#endif

#include <ctype.h>
#if HAVE_ERR
#include <err.h>
#endif
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <syslog.h>
#include <unistd.h>

#include "hq.h"

int errors;
char *raw_data;
size_t raw_size;
size_t raw_off;

void
init_buf(char *buf, size_t sz)
{
	raw_data = buf;
	raw_size = sz;
	raw_off = 0;
}

struct dom_elem *
alloc_elem(void)
{
	struct dom_elem *e;

	if ((e = calloc(1,sizeof(struct dom_elem))) == NULL)
		err(1, "calloc");
	TAILQ_INIT(&e->attrs);
	TAILQ_INIT(&e->children);
	return(e);
}

struct attr_elem *
alloc_attr(void)
{
	struct attr_elem *a;

	if ((a = calloc(1,sizeof(struct attr_elem))) == NULL)
		err(1, "calloc");
	return(a);
}

int
is_top(struct dom_elem *e)
{
	if (e == e->parent)
		return(1);
	return(0);
}

char *
extract_str(char *start, char *end)
{
	char *str;
	if (start == NULL || end == NULL)
		return(NULL);
	if (start > end)
		return(NULL);
	if ((str = calloc(1,end-start+1)) == NULL)
		return(NULL);
	memcpy(str,start,(end-start));
	return(str);
}

int
yyerror(const char *fmt, ...)
{
	va_list ap;
	char *msg;

	errors++;
	va_start(ap,fmt);
	if (vasprintf(&msg, fmt,ap) == -1)
		fatal("yyerror vasprintf");
	va_end(ap);
	free(msg);
	return(0);
}

int
lgetc(int notused)
{
	int c;
	if (raw_off >= raw_size) {
		return(EOF);
	}
	c = raw_data[raw_off];
	raw_off++;
	return(c);
}

int
lungetc(int notused)
{
	if (raw_off == 0)
		return(0);
	raw_off--;
	return(0);
}

int
peek_back(void)
{
	if (raw_off == 0)
		return(0);
	return(raw_data[raw_off-2]);
}

struct sel_attr *
alloc_sel_attr(void)
{
	struct sel_attr *a;

	if ((a = calloc(1,sizeof(struct sel_attr))) == NULL)
		err(1, "calloc");
	return(a);
}

struct sel *
alloc_sel(void)
{
	struct sel *e;

	if ((e = calloc(1,sizeof(struct sel))) == NULL)
		err(1, "calloc");
	TAILQ_INIT(&e->attrs);
	return(e);
}

struct sel_attr *
find_attr(struct sel *s, char *name)
{
	struct sel_attr *a;
	TAILQ_FOREACH(a, &s->attrs, next) {
		if (strcasecmp(name,a->name) == 0)
			return(a);
	}
	return(NULL);
}

int
unterminated_element(char *name)
{
	if (
//		((strcasecmp("li", name)) == 0) ||
		((strcasecmp("br", name)) == 0) ||
		((strcasecmp("hr", name)) == 0) ||
		((strcasecmp("img", name)) == 0) ||
		((strcasecmp("link", name)) == 0) ||
		((strcasecmp("meta", name)) == 0)
		) {
		return(1);
	}
	return(0);
}

struct dom_elem *
prev_elem(struct dom_elem *e)
{
	struct dom_elem *p;
	if (e == NULL)
		return(NULL);

	if (e == TAILQ_FIRST(e->head))
		return(NULL);
	p = TAILQ_PREV(e, domhead, next);
	return(p);
}

struct dom_elem *
next_elem(struct dom_elem *e)
{
	struct dom_elem *p;
	if (e == NULL)
		return(NULL);
	if (e == TAILQ_LAST(e->head,domhead))
		return(NULL);
	p = TAILQ_NEXT(e, next);
	return(p);
}

/* 
 * update string in place.
 * remove any leading or trailing space, tabs, newlines
 * remove internal new lines
 * replace multiple spaces/tabs with single space
 */
char *
clean_str(char *str)
{
	char *h, *p, *t;
	h = p = str;
	/* skip leading spaces and newlines*/
	while (isspace(*p)) {
		p++;
	}
	while (*p != '\0') {
		while (isspace(*p) && *p != '\0') {
			p++;
		}
		if (*p == '\0') {
			*h='\0';
		} else {
			t = p-1;
			if (*t == ' ' || *t == '\t' || *t == '\n') {
				*h = ' '; 
				h++;
			}
			*h = *p;
			h++;
			p++;
		}
	}
	*h='\0';
	return(str);
}
