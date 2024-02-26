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

#if HAVE_ERR
#include <err.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "hq.h"

int check_element(struct dom_elem *e, struct selhead *sh, int);
int match_sel(struct dom_elem *e, struct selhead *sh, int);

int
modify_dom(struct domhead *dh, struct selhead *sh, int f)
{
	struct dom_elem *e;
	TAILQ_FOREACH(e, dh, next) {
		/* don't do anything with the return value */
		check_element(e, sh, f);
	}
	return(0);
}


/* recurse */
int
check_element(struct dom_elem *e, struct selhead *sh, int f)
{
	struct dom_elem *c;
	int rc = 0;
	// check to see e matches
	if ( match_sel(e, sh, f) == 1) {
		e->match = 1;
		rc++;
	}
	TAILQ_FOREACH(c, &e->children, next) {
		rc += check_element(c, sh, f);
	}
	return(rc);
}

/*
 * return  0 = no match, 1 = match
 *
 * NOTE: the match algorithim will always match the ELEMENT.  If TEXT or
 * COMMENT is specified to limit the output the print_dom() function will
 * perform the output filtering.
 *
 * This function is in 2 parts.  First will match the element by name and
 * options.  Second will unmatch elements that do not match the element
 * selection EOP_* criteria.
 *
 */
int
match_sel(struct dom_elem *e, struct selhead *sh, int f)
{
	struct dom_elem *c;
	struct attr_elem *ea;
	struct sel *s, *sp;
	struct sel_attr *a;
	char *p, *q;
	ssize_t off;
	int rc = 0;
	int match = 0;
	int cnt = 0;
	int cmp = 0;

	if (e->type == DOMF_COMM) {
		/* mark if flagged or if parent matches */
		if (f & FLAG_COMMENT || e->parent->match == 1) {
			s = TAILQ_FIRST(sh);
	   		if (strcmp("*", s->elem) == 0 ||
				s->elem[0] == '\0' ||
				e->parent->match == 1) {
//				strcasecmp(s->elem, e->parent->name) == 0) {
					return(1);
			}
		}
		return(0);
	}

	if (e->type == DOMF_TEXT) {
		/* mark if flagged or if parent matches */
		if (f & FLAG_TEXT || e->parent->match == 1) {
			s = TAILQ_FIRST(sh);
	   		if (strcmp("*", s->elem) == 0 ||
				s->elem[0] == '\0' ||
				e->parent->match == 1) {
//				strcasecmp(s->elem, e->parent->name) == 0) {
					return(1);
			}
		}
		return(0);
	}

	// check for name match
	TAILQ_FOREACH(s, sh, next) {
		if (strcmp("*",s->elem) == 0 ||
			strcasecmp(s->elem, e->name) == 0) {
			// no selector attributes.  just match on names. simple match.
			if (TAILQ_EMPTY(&s->attrs)) {
				match = 1;
				goto found;
			}
			// no element attributes, but we have selection attributes. we can never match
			if (TAILQ_EMPTY(&e->attrs)) {
				match = 0;
				goto found;
			}
			// NOTE: class and id are also attributes. Because of this there can be more than
			// one attribute.  All selector attributes must match in order for the element to
			// be considred matching.
			//
			// loop through all attributes for match
			TAILQ_FOREACH(ea, &e->attrs, next) { // check all attribures on e
				TAILQ_FOREACH(a, &s->attrs, next) { // this will typically loop only once unless class/id is specified
					if (strcasecmp(ea->key,a->name) != 0) { // key doesn match, skip to next
						cnt++;
						continue;
					}
					switch(a->op) {
						case OP_EQ:
							cmp = strcasecmp(ea->value,a->val);
							if (cmp == 0)
								rc++;
							break;
						case OP_EQ_START:
							cmp = strcasecmp(ea->value,a->val);
							if (cmp != 0 ) {
								if (asprintf(&q,"%s-",a->val) == -1)
									err(1,"asprintf");
								p = strcasestr(ea->value,q);
								if (p != NULL && p == ea->value)
									rc++;
								free(q);
							} else
								rc++;
							break;
						case OP_START:
							cmp = strcasecmp(ea->value,a->val);
							p = strcasestr(ea->value, a->val);
							if (cmp == 0 && p != NULL && p == ea->value)
								rc++;
							break;
						case OP_END:
							cmp = strcasecmp(ea->value,a->val);
							p = strcasestr(ea->value, a->val);
							off = strlen(ea->value) - strlen(a->val);
							if (cmp == 0 && p != NULL && p == (ea->value + off))
								rc++;
							break;
						case OP_CONTAINS: // not exactly correct
						case OP_SUBSTR:
							cmp = strcasecmp(ea->value,a->val);
							if (cmp == 0 && (strcasestr(ea->value, a->val) != NULL))
								rc++;
							break;
						case OP_MATCH:
						default:	// include MATCH
							rc++;
							break;
					} // switch op
					cnt++;
				} // foreach ea attr
				// check to make the RC value and the count value match.
				if (cnt == rc) {	// all selectors matched
					match=1;
				}
			} // foreach sel attr
		}	// check elem name
found:
		/* if we have a pleminary match, check to see if the element op will unmatch the element */
		if (match) {
//printf("looking at op %s\n",elem_op_str[s->op]);
			switch(s->op) {
				case EOP_NEVER:
					match = 0;
					break;
				case EOP_INSIDE:	/*  div p   Selects all <p> elements inside <div> elements */
					sp = TAILQ_FIRST(sh);
					if (s != sp) {
						sp = TAILQ_PREV(s, selhead, next);
						if (is_top(e)) {
							match = 0;
						} else {
							c = e->parent;
							if (strcasecmp(c->name, sp->elem) != 0) {
								match = 0;
							}
						}
					} else {
						match = 0;
					}
					break;
				case EOP_PARENT:	/* div > p     Selects all <p> elements where the parent is a <div> element */
					sp = TAILQ_FIRST(sh);
					if (s != sp) {// not first node
						sp = TAILQ_PREV(s, selhead, next);
						if (strcasecmp(e->parent->name, sp->elem) != 0) {
							match = 0;
						}
					} else {
						match = 0;
					}
					break;
				case EOP_NEXT:	/* div + p     Selects the first <p> element that is placed immediately after <div> elements */
					sp = TAILQ_FIRST(sh);
					if (s != sp) {
						// find the previous non-TEXT element
						c = prev_elem(e);
						if (c == NULL) {
							match = 0;
						} else {
							if (strcasecmp(c->name, sp->elem) != 0) {
								match = 0;
							}
						}
					}
					break;
				case EOP_PRECED: /* p ~ ul  Selects every <ul> element that is preceded by a <p> element */
					sp = TAILQ_PREV(s,selhead,next);
					if (s != sp) {
						c = prev_elem(e);
						if (c == NULL) {
							match = 0;
						} else {
							if (strcasecmp(c->name, sp->elem) != 0) {
								match = 0;
							}
						}
					}
					break;
				case EOP_MATCH:		/* fallthrough */
				case EOP_ALL:		/* fallthrough */
				default:
					break;
			} // switch op
		} // if match
	} // foreach sel

  return(match);
}

