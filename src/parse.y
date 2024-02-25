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

%{

#include <sys/types.h>
#include <sys/queue.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "hq.h"

int                     yyparse(void);
int                     yylex(void);

typedef struct {
        union {
                int64_t         number;
                char            *string;
				struct attr_elem	*attr;
        } v;
        int lineno;
		int st_lineno;
} YYSTYPE;

struct domhead *head;
struct dom_elem *top, *cur;
int inelem;

%}
%token DOCTYPE
%token  <v.string>      STRING
%token  <v.string>      TEXT
%token  <v.string>      COMMENT
%type	<v.attr>		attr


%%

dom			: /* empty */
			| dom '<' doctype '>'
			| dom '<' '/' endelem '>'
			| dom '<' fullelem '>'
			| dom '<' comment '>'
			| dom text
			;

doctype		: DOCTYPE STRING {
		 		struct dom_elem *e;
				e = alloc_elem();
				e->head = head;
				e->name = strdup("doctype");
				e->value = $2;
				e->type = DOMF_DOCT;
				e->line = yylval.lineno;
				TAILQ_INSERT_TAIL(head, e, next);
		 	}
		 	;

comment		: COMMENT {
		 		struct dom_elem *e;
				e = alloc_elem();
				e->head = head;
				e->name = strdup("COMMENT");
				e->value = $1;
				e->type = DOMF_COMM;
				e->line = yylval.lineno;
				if (cur == NULL) {
					e->parent = e;				// top points to itself
					TAILQ_INSERT_TAIL(head, e, next);
				} else {
					TAILQ_INSERT_TAIL(&cur->children, e, next);
					e->parent = cur;
				}
			}
		 	;

text		: TEXT {
		 		struct dom_elem *e;
				e = alloc_elem();
				e->head = head;
				e->name = strdup("TEXT");
				e->value = $1;
				e->type = DOMF_TEXT;
				e->line = yylval.st_lineno;
				if (cur == NULL) {
					e->parent = e;				// top points to itself
					TAILQ_INSERT_TAIL(head, e, next);
				} else {
					TAILQ_INSERT_TAIL(&cur->children, e, next);
					e->parent = cur;
				}
	  		}

fullelem	: elem {
				/* some elements to not have closing tags so we just end*/
				if (unterminated_element(cur->name) == 1) {
					cur->flags |= ELEM_NOEND;
					cur = cur->parent;
				}
		 	}
			| elem '/' {
				cur->flags |= ELEM_INLINE;
				cur = cur->parent;
			}
		 	;

elem		: STRING {
	  			struct dom_elem *e;
				e = alloc_elem();
				e->head = head;
				e->name = $1;
				e->type = DOMF_ELEM;
				e->line = yylval.lineno;
				if (cur == NULL) {
					top = e;
					cur = e;
					e->parent = e;				// top points to itself
					TAILQ_INSERT_TAIL(head, e, next);
				} else {
					TAILQ_INSERT_TAIL(&cur->children, e, next);
					e->parent = cur;
					cur = e;
				}
	  		} elem_attrs
			;

endelem		: STRING {
		 		struct dom_elem *e;
		 		if (strcasecmp(cur->name, $1) != 0) {
					warnx("found end %s expecting %s line %d",$1,cur->name,yylval.lineno);
					e = cur;
					while (!is_top(e) && strcasecmp($1,e->name) != 0) {
							e = e->parent;
						}
					if (!is_top(e)) {
						e = e->parent;
					}
					cur = e;
				} else {
			 		cur = cur->parent;
				}
				free($1);
		 	}

elem_attrs	: /* empty */
		   	| elem_attrs STRING {
				struct attr_elem *a;			
				a = alloc_attr();
				a->key = $2;
				TAILQ_INSERT_TAIL(&cur->attrs, a, next);
			}
			| elem_attrs attr {
				TAILQ_INSERT_TAIL(&cur->attrs, $2, next);
			}
			;

attr		: STRING '=' STRING {
	  			struct attr_elem *a;
				a = alloc_attr();
				a->key = $1;
				a->value = $3;
				$$ = a;
	  		}
	  		;


%%

int
yylex(void)
{
	char *p, *st;
	int c, quotec;

	st = p = raw_data + raw_off;
	c = lgetc(0);
	/* skip whitespace */
	while (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
		if (c == '\n' || c == '\r')
			yylval.lineno++;
		c = lgetc(0);
	}

//printf("checking: %d: '%c'(%d)\n",inelem,c,c);
	if (c == EOF) {
//printf("trying to stop lex\n");
		return(0);
	}
	if (c == '<') {
		inelem = 1;
		return(c);
	}
	if (c == '>') {
		inelem = 0;
		return(c);
	}

	if (inelem) {
		switch(c) {
			case '\'':		// quoted strings
			case '"':
				st = p = raw_data + raw_off;
				quotec = c;
				while(1) {
					if ((c = lgetc(0)) == EOF)
						return(0);	// ran out of data before end of string
					if (c == '\n') {
						yylval.lineno++;
						continue;
					}
					if (c == quotec) {
						break;
					}
					p++;
				}
				if ((yylval.v.string = extract_str(st,p)) == NULL)
					fatal("yylex: extract_str");
				return(STRING);
				break;
			case '/':
			case '=':
				return(c);
				break;
			case '>':		// shouldn't be hit
				inelem = 0;
				return(c);
				break;
			case '!':	// DOCTYPE or COMMENT
				quotec = peek_back();
				if (quotec == '<') {
					if ((c = lgetc(0)) == '-' && (c = lgetc(0)) == '-') { // COMMENT
						// read all comments
						st = p = raw_data + raw_off;
						while(c != EOF) {
							if (c == '-' && (c = lgetc(0)) == '-' && (c = lgetc(0)) == '>') {
								if ((yylval.v.string = extract_str(st,p)) == NULL)
									fatal("yylex: extract_str");
								lungetc(c);
								return(COMMENT);
							}
							c = lgetc(0);
							p++;
						} 
					} else { // DOCTYPE ... maybe
						st = p = raw_data + raw_off-1;
						while (isalpha(c) && c != EOF) {
							c = lgetc(0);
							p++;
						}
						if ((yylval.v.string = extract_str(st,p)) == NULL)
									fatal("yylex: extract_str");
						if (strcasecmp(yylval.v.string,"DOCTYPE") == 0) {
							free(yylval.v.string);
							yylval.v.string = NULL;
							return(DOCTYPE);
						}
						// not doctype... just return string
						return(STRING);
					}
				}
				// just return !
				return(c);
				break;
			default:	/* string of some type */
				st = p = raw_data + raw_off-1;
				/* find end of string */
				while(c != '>' && c != ' ' && c != '\t' && c != EOF && c != '=' && c != '/') {
					c = lgetc(0);
					p++;
				}
				if ((yylval.v.string = extract_str(st,p)) == NULL)
							fatal("yylex: extract_str");
				lungetc(c);
				return(STRING);
				break;
		}	// switch(c)
	}	// inelem

	/* not in an element */
	st = p = raw_data + raw_off-1;
	yylval.st_lineno = yylval.lineno;
	while(c != '<' && c != EOF) {
		if (c == '\n')
			yylval.lineno++;
		p++;
		c = lgetc(0);
	}
	lungetc(c);
	if ((yylval.v.string = extract_str(st,p)) == NULL)
		fatal("yylex: extract_str");
	return(TEXT);
}

int
parse_dom(struct domhead *dh, char *raw, size_t sz)
{
	if (dh == NULL || raw == NULL)
		return(-1);
	if (sz <= 0)
		return(-1);
	
	head = dh;
	top = NULL;
	cur = top;
	init_buf(raw, sz);
	inelem = 0;
	errors = 0;
	yylval.lineno = 1;

//printf("size = %lu\n",raw_size);
//printf("off  = %lu\n",raw_off);

	yyparse();
	return(errors);
}
