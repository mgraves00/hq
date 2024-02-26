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

#include <sys/queue.h>

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#include "hq.h"

#define PRETTY_WIDTH	2

#define PRETTY_INDENT(_flags,_val) do {\
	int _i; \
	if (_flags & FLAG_PRETTY) { \
		for (_i=0; _i<(PRETTY_WIDTH * _val); _i++) \
			printf(" "); \
	} \
} while(0)

void print_attr(struct attr_elem *a, int);
void print_elem(struct dom_elem *e, int, int, char *);
void print_attr2(struct attr_elem *a, int);
void print_elem2(struct dom_elem *e, int);
void print_elem_flags(struct dom_elem *e);
int is_match(int match, int fmatch, int flags);

const char *elem_type_str[] = {
	"DOCTYPE",
	"ELEMENT",
	"TEXT",
	"COMMENT"
};

const char *sel_op_str[] = {
	"MATCH",
	"EQ",
	"CONTAINS",
	"EQ or START",
	"START",
	"END",
	"SUBSTR"
};

const char *elem_op_str[] = {
	"EOP_NEVER",
	"EOP_MATCH",
	"EOP_ALL",
	"EOP_INSIDE",
	"EOP_PARENT",
	"EOP_NEXT",
	"EOP_PRECEED"
};


int
is_match(int match, int fmatch, int flags)
{
	int rc = 0;
	int f = FLAG_ALL&flags;

	if (f & (FLAG_COMMENT|FLAG_TEXT)) {
		if (match && (f & FLAG_TEXT) && (fmatch & FLAG_TEXT))
			rc = 1;
		else if (match && (f & FLAG_COMMENT) && (fmatch & FLAG_COMMENT))
			rc = 1;
	} else {
		if (match)
			rc = 1;
	}
	if (flags & FLAG_DEL)	// invert the match
		rc = (rc==1?0:1);
	return(rc);
}

void
print_attr(struct attr_elem *a, int flags)
{
	printf("%s",a->key);
	if (a->value != NULL)
		printf("=\"%s\"",a->value);
	return;
}
void
print_elem(struct dom_elem *e, int flags, int rec, char *attr)
{
	struct attr_elem *a;
	struct dom_elem *c;

	if (!((flags & FLAG_TEXT) || (flags & FLAG_COMMENT) || (flags & FLAG_ATTR))) {
		PRETTY_INDENT(flags,rec);
	}
	switch(e->type) {
		case DOMF_DOCT:
			if ( is_match(e->match, FLAG_ELEM, flags) && ! (flags & FLAG_ATTR)) {
				printf("<!DOCTYPE %s>\n",e->value);
			}
			break;
		case DOMF_COMM:
			if ( is_match(e->match, FLAG_COMMENT, flags) && ! (flags & FLAG_ATTR)) {
				printf("<!-- %s -->\n",e->value);
			}
			break;
		case DOMF_TEXT:
			if ( is_match(e->match, FLAG_TEXT, flags) && ! (flags & FLAG_ATTR)) {
				if (flags & FLAG_PRETTY) {
					clean_str(e->value);
				}
				printf("%s",e->value);
				if (flags & FLAG_PRETTY)
					printf("\n");
			}
			break;
		case DOMF_ELEM:
			if ( is_match(e->match, FLAG_ELEM, flags)) {
				if (! (flags & FLAG_ATTR)) {
					printf("<%s",e->name);
					TAILQ_FOREACH(a, &e->attrs, next) {
						printf(" ");
						print_attr(a, flags);
					}
					if (e->flags & ELEM_INLINE) {
						printf(" />\n");
					} else {
						printf(">");
						if ((flags & ELEM_NOEND) != ELEM_NOEND)
							printf("\n");
					}
				} else {
					int f = 0;
					char *p, *s = NULL, *last;
					if ((s = strdup(attr)) == NULL)
						err(1,"strdup");
					for ((p = strtok_r(s,",",&last)); p!=NULL; (p = strtok_r(NULL,",",&last))) {
						TAILQ_FOREACH(a, &e->attrs, next) {
							if (strcasecmp(p, a->key) == 0) {
								print_attr(a,flags);
								printf(" ");
								f = 1;
							}
						}
					} 
					if (f != 0)
						printf("\n");
					if (s)
						free(s);
				}
			}
			break;
		default:
			break;
	}
	TAILQ_FOREACH(c, &e->children, next) {
		print_elem(c, flags, rec+1, attr);
	}
	if ( is_match(e->match, FLAG_ELEM, flags) && ! (flags & FLAG_ATTR)) {
		if (e->type == DOMF_ELEM) {
			if (unterminated_element(e->name) == 0) {
				if (! (e->flags & ELEM_INLINE)) {
					PRETTY_INDENT(flags,rec);
					printf("</%s>\n",e->name);
				}
			}
		}
	}
}

void
print_attr2(struct attr_elem *a, int flags)
{
	printf("\t attribute:\n");
	printf("\t\t key: %s\n",a->key);
	printf("\t\t val: %s\n",a->value);
}
void
print_elem2(struct dom_elem *e, int flags)
{
	struct attr_elem *a;
	struct dom_elem *c;
	// print element info
	printf("%s%s line %d\n",elem_type_str[e->type],(e->match==1?"*":""),e->line);
	printf("\t name: %s\n",e->name);
	if (is_top(e) || e->parent == NULL) {
		printf("\t parent: top\n");
	} else {
		printf("\t parent: %s\n",e->parent->name);
	}
	printf("\t flags: "); print_elem_flags(e);
	if (e->value != NULL) {
		printf("\t value: %s\n",e->value);
	}
	// print all attributes
	TAILQ_FOREACH(a, &e->attrs, next) {
		print_attr2(a, flags);
	}
	// print children
	TAILQ_FOREACH(c, &e->children, next) {
		print_elem2(c, flags);
	}
}

void
print_dom(struct domhead *dh, int flags, char *attr)
{
	struct dom_elem *e;

	TAILQ_FOREACH(e, dh, next) {
		if (flags & FLAG_X)
			print_elem2(e,flags);
		else
			print_elem(e,flags,0,attr);
	}
	return;
}

void
print_elem_flags(struct dom_elem *e)
{
	int c = 0;
	if (e->flags == 0) {
		printf("''\n");
		return;
	}
	if (e->flags & ELEM_INLINE) {
		printf("INLINE");
		c = 1;
	}
	if (e->flags & ELEM_NOEND) {
		if (c == 1)
			printf("|");
		printf("NOEND");
		c = 1;
	}
	printf("\n");
	return;
}

void
print_sel(struct selhead *sh)
{
	struct sel *s;
	struct sel_attr *a;

	TAILQ_FOREACH(s, sh, next) {
		switch(s->op) {
			case EOP_ALL:
				printf(","); break;
			case EOP_INSIDE:
				printf(" "); break;
			case EOP_PARENT:
				printf(">"); break;
			case EOP_NEXT:
				printf("+"); break;
			case EOP_PRECED:
				printf("~"); break;
			case EOP_NEVER:	/* fallthrough */
			default:
				break;
		}
		printf("%s",s->elem);
		if ((a = find_attr(s,"class")) != NULL) {
			printf(".%s",a->val);
		}
		if ((a = find_attr(s,"id")) != NULL) {
			printf("#%s",a->val);
		}
		TAILQ_FOREACH(a, &s->attrs, next) {
			if (strcasecmp("class",a->name) == 0)
				continue;
			if (strcasecmp("id",a->name) == 0)
				continue;
			printf("[%s",a->name);
			switch(a->op) {
				case OP_EQ:
					printf("="); break;
				case OP_CONTAINS:
					printf("~="); break;
				case OP_EQ_START:
					printf("|="); break;
				case OP_START:
					printf("^="); break;
				case OP_END:
					printf("$="); break;
				case OP_SUBSTR:
					printf("*="); break;
				default:
					break;
			}
			printf("%s]",a->val);
		}
	}
	printf("\n");
}

