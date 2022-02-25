/*
 * Copyright (c) 2022 Brian Callahan <bcallah@openbsd.org>
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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct scores {
	char name[32];
	size_t score;
} scores_t;

int
main(void)
{
	FILE *fp;
	scores_t scores[10];
	int i, j = 0, new = 0;
	const char *e;
	char *line = NULL, *l, *query;
	char name[32];
	size_t linesize = 0, score;
	ssize_t linelen;

	if (pledge("stdio rpath wpath cpath", NULL) == -1)
		return 1;

	if ((fp = fopen("../htdocs/scores.txt", "r")) == NULL)
		return 1;

	while ((linelen = getline(&line, &linesize, fp)) != -1) {
		l = line;
		l[linelen - 1] = '\0';
		(void) memset(scores[j].name, 0, sizeof(scores[j].name));
		for (i = 0; i < sizeof(scores[j].name) - 1; i++) {
			if (*l == '\t' || *l == '\0')
				break;
			scores[j].name[i] = *l++;
		}
		if (*l == '\0')
			goto bad;
		scores[j++].score = strtonum(++l, 0, LLONG_MAX, &e);
		if (e != NULL) {
bad:
			(void) fclose(fp);
			free(line);
			return 1;
		}
		if (j == 10)
			break;
	}
	free(line);
	line = NULL;
	l = NULL;

	(void) fclose(fp);

	query = getenv("QUERY_STRING");
	if (!strcmp(query, "")) {
		puts("Status: 200 OK\r");
		puts("Content-Type: application/json\r");
		puts("Access-Control-Allow-Origin: *\r");
		puts("\r");
		puts("{\n\t\"scores\": {");
		for (i = 0; i < 10; i++) {
			printf("\t\t\"%s\": %zu", scores[i].name, scores[i].score);
			if (i != 9)
				printf(",");
			printf("\n");
		}
		puts("\t}\n}");

		return 0;
	}

	i = 0;
	(void) memset(name, 0, sizeof(name));
	while (*query != '=') {
		if (*query == '\0')
			return 1;
		name[i++] = *query++;
		if (i == 30)
			break;
	}
	if (*++query == '\0')
		return 1;
	score = strtonum(query, 0, LLONG_MAX, &e);
	if (e != NULL)
		return 1;

	if ((fp = fopen("../htdocs/scores.txt", "w+")) == NULL)
		return 1;

	for (i = 0; i < 10; i++) {
		if (score > scores[i].score) {
			if (new == 0) {
				(void) fprintf(fp, "%s\t%zu\n", name, score);
				++i;
				new = 1;
			}
		}
		if (i < 10)
			(void) fprintf(fp, "%s\t%zu\n", scores[i - new].name, scores[i - new].score);
	}

	(void) fclose(fp);

	puts("Status: 200 OK\r");
	puts("Content-Type: application/json\r");
	puts("Access-Control-Allow-Origin: *\r");
	puts("\r");
	puts("{\n\t\"update\": \"success\"\n}");

	return 0;
}
