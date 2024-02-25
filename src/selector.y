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
#include <sys/socket.h>
#include <sys/stat.h>

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

// second yacc parser. rename duplicate functions / variables
#ifdef YYPREFIX
#undef YYPREFIX
#endif
#define YYPREFIX "sel"
#define yylex sel_yylex
#define yylval sel_lval
#define yyparse sel_parse
#define yydebug sel_debug
#define yynerrs sel_nerrs
#define yyerrflag sel_errflag
#define yychar sel_char
#define yyss sel_ss
#define yyssp sel_ssp
#define yyvs sel_vs
#define yyvsp sel_vsp
#define yydefred sel_defred
#define yyrule sel_rule
#define yylen sel_len
#define yyname sel_name
#define yyval sel_val
#define yylhs sel_lhs
#define yygindex sel_gindex
#define yycheck sel_check
#define yytable sel_table
#define yydgoto sel_dgoto
#define yysslim sel_sslim
#define yysindex sel_sindex
#define yyrindex sel_rindex
#define yystacksize sel_stacksize
#define YYSTYPE SELSTYPE
#define yyaccept sel_accept
#define yyabort sel_abort
#define yyoverflow sel_overflow


int                     yylex(void);

typedef struct {
        union {
                int64_t         number;
                char            *string;
				struct sel_attr	*attr;
				struct sel		*el;
        } v;
} YYSTYPE;

struct selhead *selhead;

%}
%token  <v.string>      STRING
%type	<v.number>		op;
%type	<v.attr>		filter;
%type	<v.attr>		class;
%type	<v.attr>		id;
%type	<v.el>			element;


%%

sel			:  /* empty */ {
				struct sel *s;
				s = alloc_sel();
				s->elem = strdup("");
				s->op = EOP_MATCH;
				TAILQ_INSERT_TAIL(selhead, s, next);
			}
	  		| element {
				TAILQ_INSERT_TAIL(selhead, $1, next);
				$1->op = EOP_MATCH;
	  		}
	  		| element ' ' element {					/* select all $3 inside $1 */
				TAILQ_INSERT_TAIL(selhead, $1, next);
				TAILQ_INSERT_TAIL(selhead, $3, next);
				$1->op = EOP_NEVER;
				$3->op = EOP_INSIDE;
			}
	  		| element optspc ',' optspc element {	/* select all $1 and $5 */
				TAILQ_INSERT_TAIL(selhead, $1, next);
				TAILQ_INSERT_TAIL(selhead, $5, next);
				$1->op = EOP_ALL;
				$5->op = EOP_ALL;
			}
	  		| element optspc '>' optspc element {	/* select all $5 where parent is $1 */
				TAILQ_INSERT_TAIL(selhead, $1, next);
				TAILQ_INSERT_TAIL(selhead, $5, next);
				$1->op = EOP_NEVER;
				$5->op = EOP_PARENT;
			}
	  		| element optspc '+' optspc element {	/* select first $5 placed immediately after $5 */
				TAILQ_INSERT_TAIL(selhead, $1, next);
				TAILQ_INSERT_TAIL(selhead, $5, next);
				$1->op = EOP_NEVER;
				$5->op = EOP_NEXT;
			}
	  		| element optspc '~' optspc element {	/* select every $1 that is before a $5 */
				TAILQ_INSERT_TAIL(selhead, $1, next);
				TAILQ_INSERT_TAIL(selhead, $5, next);
				$1->op = EOP_NEVER;
				$5->op = EOP_PRECED;
			}
			;

optspc		: /* empty */
			| ' '
			;

element		: '*' {
				struct sel *s;
				s = alloc_sel();
				s->elem = strdup("*");
				$$ = s;
			}
		 	| id {
				struct sel *s;
				s = alloc_sel();
				s->elem = strdup("*");
				TAILQ_INSERT_TAIL(&s->attrs, $1, next);
				$$ = s;
			}
			| class {
				struct sel *s;
				s = alloc_sel();
				s->elem = strdup("*");
				TAILQ_INSERT_TAIL(&s->attrs, $1, next);
				$$ = s;
			}
			| filter {
				struct sel *s;
				s = alloc_sel();
				s->elem = strdup("*");
				TAILQ_INSERT_TAIL(&s->attrs, $1, next);
				$$ = s;
			}
			| class filter {
				struct sel *s;
				s = alloc_sel();
				s->elem = strdup("*");
				TAILQ_INSERT_TAIL(&s->attrs, $1, next);
				TAILQ_INSERT_TAIL(&s->attrs, $2, next);
				$$ = s;
			}
			| STRING {
				struct sel *s;
				s = alloc_sel();
				s->elem = $1;
				$$ = s;
			}
			| STRING id {
				struct sel *s;
				s = alloc_sel();
				s->elem = $1;
				TAILQ_INSERT_TAIL(&s->attrs, $2, next);
				$$ = s;
			}
			| STRING class {
				struct sel *s;
				s = alloc_sel();
				s->elem = $1;
				TAILQ_INSERT_TAIL(&s->attrs, $2, next);
				$$ = s;
			}
			| STRING filter {
				struct sel *s;
				s = alloc_sel();
				s->elem = $1;
				TAILQ_INSERT_TAIL(&s->attrs, $2, next);
				$$ = s;
			}
			| STRING class filter {
				struct sel *s;
				s = alloc_sel();
				s->elem = $1;
				TAILQ_INSERT_TAIL(&s->attrs, $2, next);
				TAILQ_INSERT_TAIL(&s->attrs, $3, next);
				$$ = s;
			}
			;

class		: '.' STRING {
	 			struct sel_attr *s;
				s = alloc_sel_attr();
				s->name = strdup("class");
				s->op = OP_MATCH;
				s->val = $2;
	   			$$ = s;
			}
			;

id			: '#' STRING {
	 			struct sel_attr *s;
				s = alloc_sel_attr();
				s->name = strdup("id");
				s->op = OP_MATCH;
				s->val = $2;
	 			$$ = s;
	 		}
			;

filter		: '[' STRING ']' {
				struct sel_attr *s;
				s = alloc_sel_attr();
				s->name = $2;
				s->op = OP_MATCH;
				$$ = s;
			}
			| '[' STRING op STRING ']'{
				struct sel_attr *s;
				s = alloc_sel_attr();
				s->name = $2;
				s->val = $4;
				s->op = $3;
				$$ = s;
			}
			| '[' STRING op '"' STRING '"' ']'{
				struct sel_attr *s;
				s = alloc_sel_attr();
				s->name = $2;
				s->val = $5;
				s->op = $3;
				$$ = s;
			}
			;

op			: '=' { $$ = OP_EQ; }
	 		| '~' '=' { $$ = OP_CONTAINS; }
	 		| '|' '=' { $$ = OP_EQ_START; }
	 		| '^' '=' { $$ = OP_START; }
	 		| '$' '=' { $$ = OP_END; }
	 		| '*' '=' { $$ = OP_SUBSTR; }
			;

%%

int
yylex(void)
{
	char *p, *st;
	int c, quotec;

	st = p = raw_data + raw_off;
	c = lgetc(0);
//printf("c = '%c'\n",c);
	/* end of buffer */
	if (c == EOF) {
		return(0);
	}
	/* quoted string */
	if (c == '"' || c == '\'' || c == EOF) {
		quotec = c;
		c = lgetc(0);
		while (c != quotec) {
			p++;
			c = lgetc(0);
		}
		st++; p++;
		if ((yylval.v.string = extract_str(st,p)) == NULL)
			fatal("%slex: extract_str",YYPREFIX);
		return(STRING);
	}
	/* compress multiple spaces into single space */
	if ( c == ' ' || c == '\t' ) {
		c = lgetc(0);
		while (c == ' ' || c == '\t')
			c = lgetc(0);
		lungetc(c);
		return(' ');
	}
	/* if not alphanum just return */
	if (!isalnum(c)) {
		return(c);
	}
	/* unquoted string */
	while (isalnum(c)) {
//printf("c '%c' '%c'\n",c,*p);
		p++;
		c = lgetc(0);
	}
	if (c != EOF)
		lungetc(c);
	if ((yylval.v.string = extract_str(st,p)) == NULL)
		fatal("%slex: extract_str",YYPREFIX);
	return(STRING);
}

int
parse_sel(struct selhead *sh, char *raw)
{
	if (sh == NULL || raw == NULL)
		return(-1);
	
	selhead = sh;
	init_buf(raw,strlen(raw));
	errors = 0;

	yyparse();
	return(errors);
}

