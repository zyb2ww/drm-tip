/*
 * Copyright (c) 2006, Al Viro.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>

char *prefix1 = "a/", *prefix2 = "b/";
char *old_prefix = "O:";
char *user_pattern;

char *line;

void die(char *s)
{
	fprintf(stderr, "remap: %s\n", s);
	exit(1);
}

void Enomem(void)
{
	die("out of memory");
}

void Eio(void)
{
	die("IO error");
}

enum {SIZE = 4096};

char *buffer;
int get_line(int fd)
{
	static char *end, *end_buffer, *next;
	static size_t size;
	ssize_t count;

	if (!buffer) {
		next = end = buffer = malloc(size = SIZE);
		if (!buffer)
			Enomem();
		end_buffer = buffer + SIZE;
	}
	line = next;

	while (1) {
		if (next < end) {
			next = memchr(next, '\n', end - next);
			if (next) {
				*next++ = '\0';
				return 1;
			}
		}
		if (end == end_buffer) {
			size_t n = line - buffer;
			if (n >= SIZE) {
				n -= n % SIZE;
				memmove(line - n, line, end - line);
				line -= n;
				end -= n;
			} else {
				char *p = malloc(size *= 2);
				if (!p)
					Enomem();
				memcpy(p + n, line, end - line);
				line = p + n;
				end = p + (end - buffer);
				free(buffer);
				buffer = p;
				end_buffer = p + size;
			}
		}
		next = end;
		count = read(fd, end, end_buffer - end);
		if (!count)
			break;
		if (count < 0)
			Eio();
		end += count;
	}

	*end = '\0';
	return line != end;
}

/* to == 0 -> deletion */
struct range_map {
	int from, to;
};

struct file_map {
	char *name;
	size_t name_len;
	struct file_map *next;
	char *new_name;
	int count;
	int allocated;
	int last;
	struct range_map ranges[];
};

struct file_map *alloc_map(char *name)
{
	struct file_map *map;

	map = malloc(sizeof(struct file_map) + 16 * sizeof(struct range_map));
	if (!map)
		Enomem();
	map->name_len = strlen(name);
	map->name = map->new_name = malloc(map->name_len + 1);
	if (!map->name)
		Enomem();
	memcpy(map->name, name, map->name_len + 1);
	map->count = 0;
	map->allocated = 16;
	map->next = NULL;
	map->last = 0;
	return map;
}

/* this is 32bit FNV1 */
uint32_t FNV_hash(char *name, size_t len)
{
	uint32_t n = 0x811c9dc5;
	while (len--) {
		unsigned char c = *name++;
		n *= 0x01000193;
		n ^= c;
	}
	return n;
}

struct file_map *hash[1024];

int hash_map(struct file_map *map)
{
	size_t len = map->name_len;
	char *name = map->name;
	int n = FNV_hash(name, len) % 1024;
	struct file_map **p = &hash[n];

	while (*p) {
		if ((*p)->name_len == len && !memcmp((*p)->name, name, len))
			return 0;
		p = &(*p)->next;
	}
	*p = map;
	if (map->new_name && !map->count)
		return 0;
	if (map->new_name && map->ranges[0].from != 1)
		return 0;
	return 1;
}

struct file_map *find_map(char *name, size_t len)
{
	static struct file_map *last = NULL;
	int n = FNV_hash(name, len) % 1024;
	struct file_map *p;

	if (last && last->name_len == len && !memcmp(last->name, name, len))
		return last;

	for (p = hash[n]; p; p = p->next)
		if (p->name_len == len && !memcmp(p->name, name, len))
			break;
	if (p)
		last = p;
	return p;
}

void parse_map(char *name)
{
	struct file_map *map = NULL;
	struct range_map *range;
	char *s;
	int fd;

	fd = open(name, O_RDONLY);
	if (fd < 0)
		die("can't open map");
	while (get_line(fd)) {
		if (line[0] == 'D') {
			if (map && !hash_map(map))
				goto Ebadmap;
			if (line[1] != ' ')
				goto Ebadmap;
			if (strchr(line + 2, ' '))
				goto Ebadmap;
			map = alloc_map(line + 2);
			map->new_name = NULL;
			continue;
		}
		if (line[0] == 'M') {
			if (map && !hash_map(map))
				goto Ebadmap;
			if (line[1] != ' ')
				goto Ebadmap;
			s = strchr(line + 2, ' ');
			if (!s)
				goto Ebadmap;
			*s++ = '\0';
			if (strchr(s, ' '))
				goto Ebadmap;
			map = alloc_map(line + 2);
			if (strcmp(line + 2, s)) {
				map->new_name = strdup(s);
				if (!map->new_name)
					Enomem();
			}
			continue;
		}
		if (!map || !map->new_name)
			goto Ebadmap;
		if (map->count == map->allocated) {
			int n = 2 * map->allocated;
			map = realloc(map, sizeof(struct file_map) +
					   n * sizeof(struct range_map));
			if (!map)
				Enomem();
			map->allocated = n;
		}
		range = &map->ranges[map->count++];
		if (sscanf(line, "%d %d%*c", &range->from, &range->to) != 2)
			goto Ebadmap;
		if (range > map->ranges && range->from <= range[-1].from)
			goto Ebadmap;
	}
	if (map && !hash_map(map))
		goto Ebadmap;
	close(fd);
	return;
Ebadmap:
	die("bad map");
}

struct range_map *find_range(struct file_map *map, int l)
{
	struct range_map *range = &map->ranges[map->last];
	struct range_map *p;

	if (range->from <= l) {
		p = &map->ranges[map->count - 1];
		if (p->from > l) {
			for (p = range; p->from <= l; p++)
				;
			p--;
		}
	} else {
		for (p = map->ranges; p->from <= l; p++)
			;
		p--;
	}
	map->last = p - map->ranges;
	return p;
}

char *strnstr(char *haystack, char *needle, ssize_t len)
{
	ssize_t nl = strlen(needle), i;

	for (i = 0; i <= len - nl; i++) {
		if (!memcmp(haystack + i, needle, nl))
			return haystack + i;
	}

	return NULL;
}

void map_pattern(char *patt, char *start, char *end, struct file_map *map)
{
	size_t plen = strlen(patt);

	while (end > start) {
		struct range_map *range;
		unsigned long l;
		char *s, *more;

		s = strnstr(start, patt, end - start);
		if (!s)
			break;

		s += plen;
		printf("%.*s", (int)(s - start), start);
		start = s;

		l = strtoul(s, &more, 10);
		if (more == s || !l || l > INT_MAX)
			continue;

		if (map->new_name && (range = find_range(map, l))->to)
			l += range->to - range->from;

		printf("%lu", l);
		start = s = more;
	}

	printf("%.*s", (int)(end - start), start);
}

void mapline(char *patt)
{
	char *s = line, *start = line, *end = line - 1, *sp = line - 1;
	struct file_map *last_mapped = NULL;

	while (1) {
		struct file_map *map;
		struct range_map *range;
		unsigned long l;
		char *more;

		end = strchr(end + 1, ':');
		if (!end)
			break;

		if (sp < s)
			sp = strchr(s, ' ');

		while (sp && sp < end) {
			s = sp + 1;
			sp = strchr(s , ' ');
		}

		l = strtoul(end + 1, &more, 10);
		if (more == end + 1 || !l || l > INT_MAX)
			continue;

		map = find_map(s, end - s);
		if (!map)
			continue;

		if (patt)
			map_pattern(patt, start, s, map);

		if (map->new_name && (range = find_range(map, l))->to) {
			l += range->to - range->from;
			printf("%s:%lu", map->new_name, l);
		} else {
			printf("%s%.*s", old_prefix, (int)(more - s), s);
		}
		start = s = more;
		last_mapped = map;
	}

	if (patt && start != line)
		map_pattern(patt, start, start + strlen(start), last_mapped);
	else
		printf("%s", start);

	printf("\n");
}

int parse_hunk(int *l1, int *l2, int *n1, int *n2)
{
	unsigned long n;
	char *s, *p;
	if (line[3] != '-')
		return 0;
	n = strtoul(line + 4, &s, 10);
	if (s == line + 4 || n > INT_MAX)
		return 0;
	*l1 = n;
	if (*s == ',') {
		n = strtoul(s + 1, &p, 10);
		if (p == s + 1 || n > INT_MAX)
			return 0;
		*n1 = n;
		if (!n)
			(*l1)++;
	} else {
		p = s;
		*n1 = 1;
	}
	if (*p != ' ' || p[1] != '+')
		return 0;
	n = strtoul(p + 2, &s, 10);
	if (s == p + 2 || n > INT_MAX)
		return 0;
	*l2 = n;
	if (*s == ',') {
		n = strtoul(s + 1, &p, 10);
		if (p == s + 1 || n > INT_MAX)
			return 0;
		*n2 = n;
		if (!n)
			(*l2)++;
	} else {
		p = s;
		*n2 = 1;
	}
	return 1;
}

void parse_diff(void)
{
	int skipping = -1, suppress = 1;
	char *name1 = NULL, *name2 = NULL;
	int from = 1, to = 1;
	int l1, l2, n1, n2;
	enum cmd {
		Diff, Hunk, New, Del, Copy, Rename, Junk
	} cmd;
	static struct { const char *s; size_t len; } pref[] = {
		[Hunk] = {"@@ ", 3},
		[Diff] = {"diff ", 5},
		[New] = {"new file ", 9},
		[Del] = {"deleted file ", 12},
		[Copy] = {"copy from ", 10},
		[Rename] = {"rename from ", 11},
		[Junk] = {"", 0},
	};
	size_t len1 = strlen(prefix1), len2 = strlen(prefix2);

	while (get_line(0)) {
		if (skipping > 0) {
			switch (line[0]) {
			case '+':
			case '-':
			case '\\':
				continue;
			}
		}
		for (cmd = 0; strncmp(line, pref[cmd].s, pref[cmd].len); cmd++)
			;
		switch (cmd) {
		case Hunk:
			if (skipping < 0)
				goto Ediff;
			if (!suppress) {
				if (!skipping)
					printf("M %s %s\n", name1, name2);
				if (!parse_hunk(&l1, &l2, &n1, &n2))
					goto Ediff;
				if (l1 > from)
					printf("%d %d\n", from, to);
				if (n1)
					printf("%d 0\n", l1);
				from = l1 + n1;
				to = l2 + n2;
			}
			skipping = 1;
			break;
		case Diff:
			if (!suppress) {
				if (!skipping)
					printf("M %s %s\n", name1, name2);
				printf("%d %d\n", from, to);
			}
			free(name1);
			free(name2);
			name2 = strrchr(line, ' ');
			if (!name2)
				goto Ediff;
			*name2 = '\0';
			name1 = strrchr(line, ' ');
			if (!name1)
				goto Ediff;
			if (strncmp(name1 + 1, prefix1, len1))
				goto Ediff;
			if (strncmp(name2 + 1, prefix2, len2))
				goto Ediff;
			name1 = strdup(name1 + len1 + 1);
			name2 = strdup(name2 + len2 + 1);
			if (!name1 || !name2)
				goto Ediff;
			skipping = 0;
			suppress = 0;
			from = to = 1;
			break;
		case New:
			if (skipping)
				goto Ediff;
			suppress = 1;
			break;
		case Del:
		case Copy:
			if (skipping)
				goto Ediff;
			printf("D %s\n", name2);
			suppress = 1;
			break;
		case Rename:
			if (skipping)
				goto Ediff;
			printf("D %s\n", name2);
			break;
		default:
			break;
		}
	}
	if (!suppress) {
		if (!skipping)
			printf("M %s %s\n", name1, name2);
		printf("%d %d\n", from, to);
	}
	return;
Ediff:
	die("odd diff");
}

int main(int argc, char **argv)
{
	char *map_name = NULL;
	char opt;
	char *arg;
	size_t len;
	for (argc--, argv++; argc; argc--, argv++) {
		if (argv[0][0] != '-') {
			map_name = argv[0];
			continue;
		}
		opt = argv[0][1];
		if (!opt)
			goto Eargs;
		arg = argv[0] + 2;
		if (!*arg) {
			if (!--argc)
				goto Eargs;
			arg = *++argv;
		}
		len = strlen(arg);
		switch (opt) {
		case 'O':
			prefix1 = malloc(len + 2);
			if (!prefix1)
				Enomem();
			memcpy(prefix1, arg, len);
			prefix1[len] = '/';
			prefix1[len + 1] = '\0';
			break;
		case 'N':
			prefix2 = malloc(len + 2);
			if (!prefix2)
				Enomem();
			memcpy(prefix2, arg, len);
			prefix2[len] = '/';
			prefix2[len + 1] = '\0';
			break;
		case 'o':
			old_prefix = arg;
			break;
		case 'p':
			user_pattern = arg;
			break;
		default:
		Eargs:
			die("bad arguments");
		}
	}

	if (!map_name) {
		parse_diff();
	} else {
		parse_map(map_name);
		buffer = NULL;
		while (get_line(0))
			mapline(user_pattern);
	}
	return 0;

}
