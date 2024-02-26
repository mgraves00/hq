/* */

#ifndef HQ_H
#define HQ_H

#define fatal(a...)	errx(1,a)

// dom structure
struct attr_elem {
	char *					key;
	char *					value;
	TAILQ_ENTRY(attr_elem)	next;
};

enum {
	DOMF_DOCT,
	DOMF_ELEM,
	DOMF_TEXT,
	DOMF_COMM,
};

extern const char *elem_type_str[];
extern const char *sel_op_str[];

#define ELEM_INLINE		0x01
#define ELEM_NOEND		0x02

TAILQ_HEAD(domhead, dom_elem);
struct dom_elem {
	uint8_t					match;
	uint8_t					type;
	int						flags;
	char *					name;
	char *					value;
	int						line;
	struct dom_elem			*parent;
	TAILQ_HEAD(,attr_elem)	attrs;
	TAILQ_HEAD(,dom_elem)	children;
	TAILQ_ENTRY(dom_elem)	next;
	struct domhead			*head;
};

#define FLAG_TEXT			0x0001
#define FLAG_COMMENT		0x0002
#define FLAG_ELEM			0x0004
/* command flags */
#define FLAG_NONE			0x0100
#define FLAG_PRETTY			0x0200
#define FLAG_DEL			0x0400
#define FLAG_ATTR			0x0800
#define FLAG_X				0x8000
#define FLAG_ALL			0x00ff
#define NOT_FLAG(f)			(FLAG_ALL^(f))


/* attribute OPs */
enum {
	OP_MATCH,
	OP_EQ,
	OP_CONTAINS,
	OP_EQ_START,
	OP_START,
	OP_END,
	OP_SUBSTR
};

/* element OPs */
enum {
	EOP_NEVER = 0,
	EOP_MATCH,
	EOP_ALL,
	EOP_INSIDE,
	EOP_PARENT,
	EOP_NEXT,
	EOP_PRECED
};

struct sel_attr {
	int	op;
	char *name;
	char *val;
	TAILQ_ENTRY(sel_attr) next;
};
struct sel {
	char *elem;
	int	op;
	int has_class;
	int has_id;
	int has_attr;
	TAILQ_HEAD(,sel_attr)	attrs;
	TAILQ_ENTRY(sel) 		next;
};

TAILQ_HEAD(selhead, sel);

/* print.c */
void print_dom(struct domhead*, int, char *);
void print_sel(struct selhead *);
/* modify.c */
int modify_dom(struct domhead *, struct selhead *, int);
/* parse.y */
int parse_dom(struct domhead*, char *, size_t);
/* selector.y */
int parse_sel(struct selhead *, char *);

/* utils.c */
struct dom_elem *alloc_elem(void);
struct attr_elem *alloc_attr(void);
struct sel *alloc_sel(void);
struct sel_attr *alloc_sel_attr(void);
struct sel_attr *find_attr(struct sel *, char *);
int is_top(struct dom_elem *);
char *extract_str(char *, char *);
void init_buf(char *, size_t );
int lgetc(int );
int lungetc(int );
int peek_back(void);
int yyerror(const char *, ...)
	__attribute__((__format__ (printf, 1, 2)))
	__attribute__((__nonnull__ (1)));
int unterminated_element(char *elem);
struct dom_elem *next_elem(struct dom_elem *);
struct dom_elem *prev_elem(struct dom_elem *);
char *clean_str(char *);

extern int errors;
extern char *raw_data;
extern size_t raw_size;
extern size_t raw_off;

extern const char *elem_op_str[];

#endif /* HQ_H */
