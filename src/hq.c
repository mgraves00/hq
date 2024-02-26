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
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "hq.h"

void usage(void);
int read_file(int fp, char **buf, size_t *buflen);
int read_stdin(int fp, char **buf, size_t *buflen);

extern char *__progname;

void
usage(void)
{
	printf("%s: [-cdhpt] [-a attr_name[,attr_name [-f html_file] css_selector\n",__progname);
	exit(1);
}

int
main(int argc, char **argv)
{
	struct domhead dh;
	struct selhead sh;
	int ch, fd;
	int flags=FLAG_NONE;
	int rc=0;
	char *fname = NULL;
	char *selector = NULL;
	char *attrname = NULL;
	char *raw = NULL;
	size_t raw_len = 0;

	while ((ch = getopt(argc, argv, "a:cdf:hptx")) != -1 ) {
		switch (ch) {
			case 'a':
				flags |= FLAG_ATTR;
				if ((attrname = strdup(optarg)) == NULL)
					errx(1,"strdup");
				break;
			case 'c':
				flags |= FLAG_COMMENT;
				break;
			case 'd':
				flags |= FLAG_DEL;
				break;
			case 'f':
				if ((fname = strdup(optarg)) == NULL)
					errx(1, "strdup");
				break;
			case 'p':
				flags |= FLAG_PRETTY;
				break;
			case 't':
				flags |= FLAG_TEXT;
				break;
			case 'x':
				flags |= FLAG_X;
				break;
			case 'h':
			default:
				usage();
				break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1) {
		usage();
	}
	if ((selector = strdup(argv[0])) == NULL)
		err(1, "strdup");
	
	if (fname[0] == '\0') {
		fd = STDIN_FILENO;
		read_stdin(fd, &raw, &raw_len);
	} else {
		if ((fd = open(fname, O_RDONLY)) == -1)
			err(1,"open");
		read_file(fd, &raw, &raw_len);
		close(fd);
	}
	
	TAILQ_INIT(&dh);
	TAILQ_INIT(&sh);

	rc = parse_sel(&sh, selector);
	if (rc != 0)
		errx(1,"bad selector");
	if (selector)
		free(selector);

	if (flags & FLAG_X) {
		printf("selector: ");
		print_sel(&sh);
	}

	rc = parse_dom(&dh, raw, raw_len);
	if (rc != 0)
		errx(1,"file parse errors");
	if (raw)
		free(raw);

	rc = modify_dom(&dh, &sh, flags);
	if (rc != 0)
		errx(1,"modify errors");

	print_dom(&dh, flags, attrname);
	if (attrname)
		free(attrname);
//	free_dom(&dh);
//	free_sel(&sh);
	return(0);
}

int
read_file(int fp, char **buf, size_t *buflen)
{
	struct stat st;
	ssize_t rlen;

	if (fstat(fp, &st) != 0) {
		err(1,"fstat");
	}
	*buflen = st.st_size;
	if ((*buf = calloc(*buflen+1,sizeof(char))) == NULL) {
		err(1,"calloc");
	}
	rlen = read(fp, *buf, *buflen);
	if ((size_t)rlen != *buflen)
		warnx("file read error: read: %ld, expected: %ld", rlen, *buflen);

	return 0;
}

int
read_stdin(int fp, char **buf, size_t *buflen)
{
	ssize_t rlen;
	char	tbuf[1024];
	size_t	off = 0;
	char	*p;

	*buf = NULL;
	bzero(tbuf,sizeof(tbuf));
	while ((rlen = read(fp, tbuf, 1024)) != -1 && rlen != 0) {
		if ((p = realloc(*buf, off+rlen)) == NULL) {
			free(*buf);
			err(1,"realloc");
		}
		*buf = p;
		bcopy(tbuf, p+off, rlen);
		off = off+rlen;
	}
	if ((p = realloc(*buf, off+1)) == NULL) {
			free(*buf);
			err(1,"realloc");
	}
	p[off+1] = '\0';
	*buf = p;
	*buflen = off;
	return(0);
}

