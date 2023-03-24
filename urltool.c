
/* Changes:
 * 20220901: splitted
 */

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "urltool.h"

static	FILE	*_slog_fp = NULL;
static	char	*_wget_proxy = NULL;
static	char	*_wget_cookies = NULL;
static  int	_session_flags = 0;

/* 
 * IE6 on Windows XP: 
 *     Mozilla/4.0 (compatible; MSIE 6.0; Microsoft Windows NT 5.1)
 * Firefox on Windows XP: 
 *     Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.14) Gecko/20080404 Firefox/2.0.0.14
 * Firefox on Ubuntu Gutsy: 
 *     Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.14) Gecko/20080418 Ubuntu/7.10 (gutsy) Firefox/2.0.0.14
 * Safari on Mac OS X Leopard: 
 *     Mozilla/5.0 (Macintosh; U; Intel Mac OS X; en) AppleWebKit/523.12.2 (KHTML, like Gecko) Version/3.0.4 Safari/523.12.2
 */
#define BROWSER		\
	"Mozilla/4.0 (compatible; MSIE 6.0; Microsoft Windows NT 5.1)"

#define MAX_RETRY	3

int urltool(void)
{
	char	key[512], *from, *to;
	int	i;

	/* url_get_tail() */
	strcpy(key, "https://e-hentai.org/g/195/3b968d$#////  ");
	i = url_trim_tail(key, "$#/ ");
	printf("url_trim_tail(): %s [%d:%d]\n", key, i, (int)strlen(key));
	printf("url_get_tail(): %s\n", 
	 		url_get_tail("https://e-hentai.org/s/7ea07158bd/1958342-5", '/'));
	strcpy(key, "https://e-hentai.org/g/1958342/016a3b968d/");
	printf("url_get_tail(): %s\n", url_get_tail(key, '/'));

	/* url_get_path() */
	url_get_path("https://e-hentai.org/g/1958342/016a3b968d/", 0, key, sizeof(key));
	printf("url_get_path(): %s\n", key);
	url_get_path("https://e-hentai.org/g/1958342/016a3b968d/", 1, key, sizeof(key));
	printf("url_get_path(): %s\n", key);
	url_get_path("https://e-hentai.org/g/1958342/016a3b968d/", 2, key, sizeof(key));
	printf("url_get_path(): %s\n", key);

	/* url_reform() */
	printf("url_reform(): %s\n", 
			url_reform("http://50.7.233.114/ehg/png&amp;t=370249&amp;n=039.png", NULL, 0));

	/* url_get_index() */
	i = url_get_index("https://e-hentai.org/s/7ea07158bd/1958342-55", key, sizeof(key));
	printf("url_get_index(): %d %s\n", i, key);
	i = url_get_index("1958342-55", key, sizeof(key));
	printf("url_get_index(): %d %s\n", i, key);

	/* strx_strrpch() */
	strcpy(key, "replace the 'old' char in the string 's' with 'new'");
	strx_strrpch(key, 0, '\'', '\"');
	printf("strx_strrpch: %s\n", key);
	strcpy(key, "replace the 'old' char in the string 's' with 'new'");
	strx_strrpch(key, 2, '\'', '\"');
	printf("strx_strrpch: %s\n", key);
	strcpy(key, "replace the 'old' char in the string 's' with 'new'");
	strx_strrpch(key, -2, '\'', '\"');
	printf("strx_strrpch: %s\n", key);

	/* strx_strstr() */
	from = strx_strstr("Each invocation of va_start() must be matched", 
			&to, "a", "()", "c", "n", NULL);
	if (from) {
		strx_puts(from, (int)(to - from));
	}
	from = strx_strstr("Each invocation of va_start() must be matched", 
			&to, "a", "()", "c", "mu", NULL);
	if (from) {
		strx_puts(from, (int)(to - from));
	}

	htm_common_pick("<meta name=\"juicyads\" content=\"0f3e4770\"/>", "\"", "\"", key, sizeof(key));
	printf("htm_common_pick: %s\n", key);
	htm_common_pick("<meta name=\"juicyads\" content=\"0f3e4770\"/>", "content=\"", "\"/>", key, sizeof(key));
	printf("htm_common_pick: %s\n", key);
	return 0;
}


/* url[] = "https://e-hentai.org/g/1958342/016a3b968d////  "
 * url_trim_tail(url, "/ ") stores "https://e-hentai.org/g/1958342/016a3b968d"
 * and return its length.*/
int url_trim_tail(char *url, char *cset)
{
	int	i = strlen(url);

	for (i--; (i >= 0) && strchr(cset, url[i]); url[i--] = 0);
	return i + 1;
}

#define	ISCTRLSP(c)	(((c) > 0) && ((c) <= ' '))
#define	ISTSET(s,c)	((s) && strchr((s),(c)))

/* same to url_trim_tail() plus removing the white spaces \x1 ~ \x20 */
int url_trim_tail_ctlsp(char *url, char *cset)
{
	int	i = strlen(url);

	for (i--; (i >= 0) && (ISCTRLSP(url[i]) || ISTSET(cset, url[i])); url[i--] = 0);
	return i + 1;
}

char *url_skip_head_ctlsp(char *url, char *cset)
{
	for ( ; *url && (ISCTRLSP(*url) || ISTSET(cset, *url)); url++);
	return url;
}


/* when 'sep' is '/':
 * return "1958342-5" from "https://e-hentai.org/s/7ea07158bd/1958342-5"
 * return "016a3b968d" from "https://e-hentai.org/g/1958342/016a3b968d/" */
char *url_get_tail(char *url, int sep)
{
	char	*p, cset[2] = { (char)sep, 0 };

	url_trim_tail_ctlsp(url, cset); /* remove the trailing '/' and white spaces */
	if ((p = strrchr(url, sep)) == NULL) {
		return url;
	}
	return p + 1;
}

/* return one of the path node. For example
 * url_get_path("https://e-hentai.org/g/1958342/016a3b968d/", 0, ...) return "e-hentai.org"
 * url_get_path("https://e-hentai.org/g/1958342/016a3b968d/", 1, ...) return "g"
 * url_get_path("https://e-hentai.org/g/1958342/016a3b968d/", 2, ...) return "1958342" */
char *url_get_path(char *url, int pn, char *buf, int blen)
{
	char	*p;
	int	i;

	if ((p = strstr(url, "://")) == NULL) {
		p = url;
	} else {
		p += 3;
	}
	for (i = 0; i < pn; i++) {
		if ((p = strchr(p, '/')) == NULL) {
			return NULL;
		}
		p++;
	}
	for (blen--, i = 0; i < blen; i++) {
		if ((p[i] == 0) || (p[i] == '/')) {
			break;
		}
		buf[i] = p[i];
	}
	buf[i] = 0;
	//printf("url_get_path [%d]: %s\n", pn, buf);
	return buf;
}

/* For wget's sake to remove the "&amp;" in the URL. Such as change
 *    http://50.7.233.114/ehg/image.php?f=5154150da59ceb92e8ce9fd13ef7bab7615cd
 *    b58-503119-892-1237-png&amp;t=370249-94283429070c8dadfba302cdbdd75f3dfb
 *    dedc12&amp;n=039.png
 * to
 *    http://50.7.233.114/ehg/image.php?f=5154150da59ceb92e8ce9fd13ef7bab7615cd
 *    b58-503119-892-1237-png;t=370249-94283429070c8dadfba302cdbdd75f3dfb
 *    dedc12;n=039.png 
 */
char *url_reform(char *url, char *buffer, int blen)
{
	static	char	tmpbuf[1024];
	int	i, k;

	if (buffer == NULL) {
		buffer = tmpbuf;
		blen   = sizeof(tmpbuf);
	}
	for (i = k = 0; url[k]; i++, k++) {
		buffer[i] = url[k];
		if (!strx_strncmp(&url[k], "&amp;")) {
			k += 4;
		}
	}
	buffer[i++] = 0;
	return buffer;
}

/* for URL like https://e-hentai.org/s/7ea07158bd/1958342-5
 * return int(5) and 1958342 in key. Otherwise return -1 */
int url_get_index(char *s, char *key, int klen)
{
	char	*p;

	s = url_get_tail(s, '/');
	if ((p = strchr(s, '-')) == NULL) {
		return -1;
	}

	if (key && (klen > 0)) {
		while (s < p) {
			*key++ = *s++;
		}
		*key = 0;
	}
	return (int) strtol(p+1, NULL, 0);
}

int url_is_remote(char *s)
{
	return (!strx_strncmp(s, "http://") || !strx_strncmp(s, "https://"));
}

int url_is_image(char *url)
{
	if ((url = strrchr(url, '.')) == NULL) {
		return 0;	/* no image in the URL */
	}
	url++;
	if (!strcmp(url, "jpg") || !strcmp(url, "jpe")) {
		return 1;
	}
	if (!strcmp(url, "gif")) {
		return 1;
	}
	if (!strcmp(url, "png")) {
		return 1;
	}
	if (!strcmp(url, "bmp")) {
		return 1;
	}
	return 0;
}

char *url_image_sn(int curr, int last)
{
	static	char	sn[16];
	
	if (last < 10) {
		sprintf(sn, "%d", curr);
	} else if (last < 100) {
		sprintf(sn, "%02d", curr);
	} else if (last < 1000) {
		sprintf(sn, "%03d", curr);
	} else if (last < 10000) {
		sprintf(sn, "%04d", curr);
	} else if (last < 100000) {
		sprintf(sn, "%05d", curr);
	} else {
		sprintf(sn, "%06d", curr);
	}
	return sn;
}

int strx_blankline(char *s)
{
	while (*s && (*s <= ' ')) s++;
	return ! *s;
}

char *strx_alloc(char *s, unsigned extra)
{
	char	*dest = malloc(strlen(s)+1+extra);

	if (dest) {
		strcpy(dest, s);
	}
	return dest;
}

char *strx_strncpy(char *dest, const char *src, size_t n)
{
	strncpy(dest, src, n-1);
	dest[n-1] = 0;
	return dest;
}

int strx_strncmp(char *dest, const char *src)
{
	return strncmp(dest, src, strlen(src));
}


/* copy contents between 'from' and 'to', not include 'to', to 'dest'
 * ended with appended '\0' */
int strx_memcpy(char *dest, int dlen, char *from, char *to)
{
	int	n = 0;

	for (n = 0; (--dlen > 0) && (from < to); n++) *dest++ = *from++;
	*dest++ = 0;
	return n;
}

void strx_puts(char *s, int n)
{
	while (n--) {
		if (*s == 0) break;
		putchar(*s++);
	}
	putchar('\n');
}

void strx_putx(char *s, int n)
{
	while (n--) {
		printf("\\%02x", (unsigned char)*s++);
	}
	putchar('\n');
}

/* replace the 'old' char in the string 's' with 'new'
 * num = 0: replace all occurance in the string
 * num > 0: replace the specified 'num' occurance in the string
 * num < 0: replace the specified 'num' occurance in the string from tail */
int strx_strrpch(char *s, int num, int old, int new)
{
	if (num == 0) {		/* unlimited search and replace */
		while (*s) {
			if (*s == old) {
				*s = new;
				num++;
			}
			s++;
		}
	} else if (num > 0) {	/* limited search and replace from left */
		while (num && *s) {
			if (*s == old) {
				*s = new;
				num--;
			}
			s++;
		}
	} else {		/* limited search and replace from right */
		char	*p = s + strlen(s) - 1;
		while (num && (p >= s)) {
			if (*p == old) {
				*p = new;
				num++;
			}
			p--;
		}
	}
	return num;
}

char *strx_strchr(char *s, char *cset)
{
	while (s && *s) {
		if (strchr(cset, *s)) {
			return s;
		}
		s++;
	}
	return NULL;
}

/* for a given string "Each invocation of va_start() must be matched"
 * strx_strstr(s, &endp, "in", "va", NULL) returns between "invocation of va"
 * strx_strstr(s, &endp, "on o", "va", "()", NULL) returns "on of va_start()"
 * anything mistching returns NULL */
char *strx_strstr(char *s, char **endp, char *first, char *last, ...)
{
	va_list	ap;
	char	*begin, *end, *next, *cp;

	while ((begin = strstr(s, first)) != NULL) {
		next = begin + strlen(first);
		if ((end = strstr(next, last)) == NULL) {
			break;
		}
		end += strlen(last);
		//strx_puts(begin, (int)(end - begin));

		va_start(ap, last);
		while ((cp = va_arg(ap, char *)) != NULL) {
			if ((next = strstr(next, cp)) == NULL) {
				break;
			}
			if (next > end) {	/* out of boundry */
				break;
			}
			next += strlen(cp);
		}
		va_end(ap);

		if (cp == NULL) {	/* matched */
			if (endp) {
				*endp = end;
			}
			//strx_puts(begin, (int)(end - begin));
			return begin;
		}
		s = end;
	}
	return NULL;
}
	

void *htm_alloc(char *fname, int exhead, int *flen)
{
	FILE	*fp;
	char	*hbuf;
	int	n;

	if ((fp = fopen(fname, "r")) == NULL) {
		perror(fname);
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	n = ftell(fp);
	rewind(fp);

	if ((hbuf = calloc(n + exhead, 1)) == NULL) {
		fclose(fp);
		return NULL;
	}
	n = fread(hbuf + exhead, 1, n, fp);
	fclose(fp);

	if (flen) {
		*flen = n;
	}
	return hbuf;
}

/* For example, s is "<td class=gdt1>File Size:</td>x...", so
 *   htm_doc_pick(s, &sp, "<td", "</td>", buf, sizeof(buf)) 
 * returns a pointer to "x..."
 * stores "File Size:" into 'buf', and points 'sp' to "<td class" 
 */
char *htm_doc_pick(char *s, char **sp, char *from, char *to, char *buf, int blen)
{
	if ((s = strstr(s, from)) == NULL) {
		return NULL;
	}

	if (sp) {	/* save and return the start point */
		*sp = s;
	}

	/* move to the end of the tag */
	if ((s = strchr(s + strlen(from), '>')) == NULL) {
		return NULL;
	}

	/* mark the starting point */
	from = url_skip_head_ctlsp(++s, NULL);		/* from = ++s; */

	if ((s = strstr(s, to)) == NULL) {	/* s moves to the end point */
		return NULL;
	}
	
	if (buf) {
		strx_memcpy(buf, blen, from, s);
		url_trim_tail_ctlsp(buf, NULL);
	} else if (blen < 0) {	/* broken mode will break the original string */
		*s = 0;
		url_trim_tail_ctlsp(from, NULL);
		return from;	/* broken mode will return the content string */
	}
	return s + strlen(to);
}


/* if 's' is
 *   <meta name="juicyads-site-verification" content="0f3e47704e352bf534e98d4d45411fda"/> 
 * then
 *   htm_tag_pick(s, "name=\"", "\"", buf, blen);
 * will return 'juicyads-site-verification' by 'buf'.
 *   htm_tag_pick(s, "name=\"", "\"", NULL, 0);
 * will return a pointer to 'juicyads-site-verification" content="0f...' to the end.
 *   htm_tag_pick(s, "name=\"", "\"", NULL, -1);
 * will return 'juicyads-site-verification' but break the original string by inserting '\0'.
 */
char *htm_tag_pick(char *s, char *from, char *to, char *buf, int blen)
{
	char	*endp;

	if ((endp = strchr(s + strlen(from), '>')) == NULL) {	/* set the picking boundry */
		return NULL;
	}
	if ((s = strstr(s, from)) == NULL) {
		return NULL;
	}
	if (s > endp) {
		return NULL;	/* hit the boundry */
	}
	from = s + strlen(from);	/* 'from' move to the copy starting point */

	if ((s = strstr(from, to)) == NULL) {
		return NULL;
	}
	if (s > endp) {
		return NULL;	/* hit the boundry */
	}

	if (buf) {
		strx_memcpy(buf, blen, from, s);
	} else if (blen < 0) {	/* broken mode will break the original string */
		*s = 0;		/* break the string */
	}
	return from;	/* return the copy starting point */
}

/* if 's' is
 *   <meta name="juicyads-site-verification" content="0f3e47704e352bf534e98d4d45411fda"/> 
 * then
 *   htm_common_pick(s, "\"", "\"", buf, blen);
 * will fill 'buf' with 'juicyads-site-verification'.
 *   htm_common_pick(s, "content=\"", "\"/>", buf, blen);
 * will fill 'buf' with 'juicyads-site-verification'.
 *   htm_common_pick(s, "content=\"", "\"/>", NULL, 0);
 * will return a pointer to '0f3e47704e352bf534e98d4d45411fda' to the end.
 *   htm_common_pick(s, "name=\"", "\"", NULL, -1);
 * will return a pointer to 'juicyads-site-verification' but break the original string by inserting '\0'.
 */
char *htm_common_pick(char *s, char *from, char *to, char *buf, int blen)
{
	/* if 'from' is null then it copies from the beginning */
	if (from == NULL) {
		from = s;
	} else {
		if ((s = strstr(s, from)) == NULL) {
			return NULL;
		}
		from = s + strlen(from);	/* 'from' move to the copy starting point */
	}

	/* if 'to' is null then it copies to the end 
	 * ignore the break mode (blen = -1) */
	if (to == NULL) {
		if (buf) {
			strx_strncpy(buf, from, blen);
		}
		return from;	/* return the copy starting point */
	}

	if ((s = strstr(from, to)) == NULL) {
		return NULL;
	}

	if (buf) {
		strx_memcpy(buf, blen, from, s);
	} else if (blen < 0) {	/* broken mode will break the original string */
		*s = 0;		/* break the string */
	}
	return from;	/* return the copy starting point */
}

/* break down the html file by its URLs */
int htm_break(char *fname)
{
	char	*hbuf, *p, *k, tmp;

	if ((hbuf = htm_alloc(fname, 0, NULL)) == NULL) {
		return -1;
	}

	p = hbuf;
	while (1) {
		if ((k = strx_strchr(p, "<{,")) == NULL) {
			break;
		}
		if (*k == ',') k++;
		tmp = *k;
		*k = 0;
		if (!strx_blankline(p)) {
			puts(p);
		}
		*k = tmp;

		p = k;
		if ((k = strx_strchr(p, ">},")) == NULL) {
			break;
		}
		tmp = *++k;
		*k = 0;
		puts(p);
		*k = tmp;
		
		p = k;
	}
	free(hbuf);
	return 0;
}


FILE *sys_pipe_read(char *cmd, ...)
{
	va_list	ap;
	char	*argv[64];
	int	i, fd[2];

	va_start(ap, cmd);
	argv[0] = cmd;
	for (i = 1; i < 64; i++) {
		argv[i] = va_arg(ap, char *);
		if (argv[i] == NULL) {
			break;
		}
	}
	va_end(ap);

	pipe(fd);

	if (fork() == 0) {
		close(fd[0]);	/* Child process closes up input side of pipe */
		
		/* redirect the standard output to pipe */
		//close(1), dup(fd[1]);	
		dup2(fd[1], 1);

		if (_wget_proxy) {
			setenv("http_proxy", _wget_proxy, 1);
			printf("Proxy: %s\n", _wget_proxy);
		} else {
			printf("Proxy: none\n");
		}
		execvp(argv[0], argv);
		return NULL;
	}

	close(fd[1]);	/* Parent process closes up output side of pipe */
	return fdopen(fd[0], "r");
}

FILE *sys_pipe_write(char *cmd, ...)
{
	va_list	ap;
	char	*argv[64];
	int	i, fd[2];

	va_start(ap, cmd);
	argv[0] = cmd;
	for (i = 1; i < 64; i++) {
		argv[i] = va_arg(ap, char *);
		if (argv[i] == NULL) {
			break;
		}
	}
	va_end(ap);

	pipe(fd);

	if (fork() == 0) {
		close(fd[1]);	/* Child process closes up output side of pipe */
		
		/* redirect the standard output to pipe */
		//close(0), dup(fd[0]);	
		dup2(fd[0], 0);

		if (_wget_proxy) {
			setenv("http_proxy", _wget_proxy, 1);
			printf("Proxy: %s\n", _wget_proxy);
		} else {
			printf("Proxy: none\n");
		}
		execvp(argv[0], argv);
		return NULL;
	}

	close(fd[0]);	/* Parent process closes up input side of pipe */
	return fdopen(fd[1], "w");
}


int sys_exec_generic(char *cmd, ...)
{
	va_list	ap;
	char	*argv[64];
	int	i, rcode;

	va_start(ap, cmd);
	argv[0] = cmd;
	for (i = 1; i < 64; i++) {
		argv[i] = va_arg(ap, char *);
		if (argv[i] == NULL) {
			break;
		}
	}
	va_end(ap);

	if (fork() == 0) {
		if (_wget_proxy) {
			setenv("http_proxy", _wget_proxy, 1);
			printf("Proxy: %s\n", _wget_proxy);
		} else {
			printf("Proxy: none\n");
		}
		execvp(argv[0], argv);
		return -1;
	}

	wait(&rcode);
	printf("%s returns: %d\n", cmd, rcode);
	return rcode;
}


/* wrapper of youtube-dl
 * proxy: username:passwd@10.20.30.40:1234 or
 * http://hniksic:mypassword@proxy.company.com:8001/ */
int sys_download_ytdl(char *url)
{
	char	*argv[64] = { "youtube-dl", NULL };
	int	i, rcode;

	for (i = 0; argv[i]; i++);

	/*if (url) {
		argv[i++] = url_reform(url, NULL, 0);
	}*/
	argv[i++] = url;
	argv[i++] = NULL;

	/*for (i = 0; argv[i]; printf("%s ", argv[i++])); 
	puts("");*/

	if (fork() == 0) {
		if (_wget_proxy) {
			setenv("http_proxy", _wget_proxy, 1);
			printf("Proxy: %s\n", _wget_proxy);
		} else {
			printf("Proxy: none\n");
		}
		execvp(argv[0], argv);
		return -1;
	}

	wait(&rcode);
	printf("Youtube-dl returns: %d\n", rcode);
	return rcode;
}


/* wrapper of ffmpeg; seems the ffmpeg will pick the highest resolution itself.
 * Download .m3u8 media:
 * https://stackoverflow.com/questions/50641251/linux-m3u8-how-to-manually-download-
 *    and-create-an-mp4-from-segments-listed-in
 * https://stackoverflow.com/questions/49975527/choose-download-resolution-in-m3u8-list-with-ffmpeg
 * https://gist.github.com/tzmartin/fb1f4a8e95ef5fb79596bd4719671b5d
 * youtube_dl -g URL to get the m3u8 url
 * */
int sys_download_m3u8(char *url, char *fname)
{
	char	*argv[64] = { "ffmpeg", "-i", NULL, "-c", "copy", "-f", "mpegts", NULL, NULL };
	int	i, rcode;

	for (i = 0; argv[i]; i++);

	/*if (url) {
		argv[i++] = url_reform(url, NULL, 0);
	}*/
	argv[i++] = url;

	for (i = 0; argv[i]; i++);
	argv[i++] = fname;

#if 0
	for (i = 0; argv[i]; printf("%s ", argv[i++])); 
	puts("");
	rcode = 0;
#else
	if (fork() == 0) {
		if (_wget_proxy) {
			setenv("http_proxy", _wget_proxy, 1);
			printf("Proxy: %s\n", _wget_proxy);
		} else {
			printf("Proxy: none\n");
		}
		execvp(argv[0], argv);
		return -1;
	}

	wait(&rcode);
	printf("FFMPEG returns: %d\n", rcode);
#endif
	return rcode;
}


/* proxy: username:passwd@10.20.30.40:1234 or
 * http://hniksic:mypassword@proxy.company.com:8001/ */
/* 20230104 Fixed the "ERROR: The certificate of `www.dropbox.com' is not trusted." issue 
 * https://stackoverflow.com/questions/9224298/how-do-i-fix-certificate-errors-when-running-
 *      wget-on-an-https-url-in-cygwin */
/* 20230131 Added cookies
 * https://stackoverflow.com/questions/1324421/how-to-get-past-the-login-page-with-wget
 * */
int sys_download_wget(char *url, char *fname)
{
	char	*wget_tbl[] = { "-O", "--load-cookies" };
	char	*argv[64] = { "wget", "-U", BROWSER, "-t", "1", "-T", "60", "--no-check-certificate", NULL };
	//char	*argv[64] = { "wget", "--load-cookies", "cookies.txt", NULL };
	int	i, rcode;

	for (i = 0; argv[i]; i++);	// i = 5;

	if (fname) {
		argv[i++] = wget_tbl[0];
		argv[i++] = fname;
	}
	if (_wget_cookies && cflags_check(CFLAGS_COOKIE)) {
		argv[i++] = wget_tbl[1];
		argv[i++] = _wget_cookies;
	}
	if (url) {
		argv[i++] = url_reform(url, NULL, 0);
	}
	argv[i++] = NULL;

	for (i = 0; argv[i]; printf("%s ", argv[i++])); 
	puts("");

	if (fork() == 0) {
		if (_wget_proxy) {
			setenv("http_proxy", _wget_proxy, 1);
			printf("Proxy: %s\n", _wget_proxy);
		} else {
			printf("Proxy: none\n");
		}
		execvp(argv[0], argv);
		//execlp("env", "env", NULL);
		return -1;
	}

	wait(&rcode);
	printf("WGET returns: %d\n", rcode);
	return rcode;
}

/* 20220629 do not using sys_download_wget_image() in webpages.
 * it will add a digit in every retry. It's fine with pic.jpg.0, pic.jpg.1, ...
 * but it will break the analyzer by 1610666-1.0, 1610666-1.1, etc */
int sys_download_wget_image(char *url, char *fname)
{
	int	i;

	for (i = 0; i < MAX_RETRY; i++) {
		if (sys_download_wget(url, fname) == 0) {
			return 0;
		}
		sleep(i*2+1);
	}
	return ERR_DL_IMAGE;
}

int sys_download_wget_page(char *url, char *fname)
{
	FILE	*fp;
	void	*backup;
	int	i, blen;

	/* delete but save a copy in memory */
	if ((backup = htm_alloc(fname, 0, &blen)) != NULL) {
		unlink(fname);
	}

	for (i = 0; i < MAX_RETRY; i++) {
		if (sys_download_wget(url, fname) == 0) {
			if (backup) {
				free(backup);
			}
			return 0;
		}
		sleep(1);
		unlink(fname);	/* avoid digit ext like 1610666-1.0 */
	}

	/* download failed, recover the original file */
	if (backup) {
		if ((fp = fopen(fname, "w")) != NULL) {
			fwrite(backup, 1, blen, fp);
			fclose(fp);
		}
		free(backup);
	}
	return ERR_DL_PAGE;
}

int sys_download_proxy_open(char *proxy)
{
	if (proxy) {
		sys_download_proxy_close();
		_wget_proxy = strx_alloc(proxy, 0);
	}
	return _wget_proxy ? 1 : 0;
}

void sys_download_proxy_close(void)
{
	if (_wget_proxy) {
		free(_wget_proxy);
	}
	_wget_proxy = NULL;
}

int sys_download_cookies_open(char *cookie)
{
	if (cookie) {
		sys_download_cookies_close();
		_wget_cookies = realpath(cookie, NULL);
	}
	return _wget_cookies ? 1 : 0;
}

void sys_download_cookies_close(void)
{
	if (_wget_cookies) {
		free(_wget_cookies);
	}
	_wget_cookies = NULL;
}




int sys_delay(void)
{
	static	int	counter = 0;

	counter = (counter + 1) % DOWNLOAD_INTERVAL;
	sleep(counter + 1);
	return counter + 1;
}



#if 0
int url_download(char *url)
{
	char	cmdbuf[1024];
	int	len;

	/* claim it as Mozilla 4.0, try once only */
	sprintf(cmdbuf, "wget -U \'%s\' -t 1 -O %s  ", BROWSER,
			e_hentai_url_filename(url, NULL, 0));
	
	strcat(cmdbuf, "'");
	len = strlen(cmdbuf);
	url_reform(url, cmdbuf + len, len);
	strcat(cmdbuf, "'");

	puts(cmdbuf);
	system(cmdbuf);
	return 0;
}
#endif

int cflags_set(int flag)
{
	return (_session_flags |= flag);
}

int cflags_clear(int flag)
{
	return (_session_flags &= ~flag);
}

int cflags_check(int flag)
{
	return ((_session_flags & flag) == flag);
}

int cflags_argvs(int c)
{
	switch (c) {
	case 's':
		cflags_clear(CFLAGS_CONTINUOUS);
		break;
	case 'k':
		cflags_set(CFLAGS_KEEP_PAGE);
		break;
	case 'a':
		cflags_set(CFLAGS_DUMP | CFLAGS_DUMP_ALL);
		cflags_clear(CFLAGS_MEDIA);
		break;
	case 'i':
		cflags_set(CFLAGS_DUMP_MEDIA);
		cflags_clear(CFLAGS_DUMP);
		cflags_clear(CFLAGS_MEDIA);
		break;
	case 'u':
		cflags_clear(CFLAGS_SORT);
		break;
	case 0:
		cflags_set(CFLAGS_DUMP);
		cflags_clear(CFLAGS_MEDIA);
		break;
	}
	return _session_flags;
}


int slog_open(char *logname)
{
	if (_slog_fp && (_slog_fp != stderr)) {
		fclose(_slog_fp);	/* already open, so close it first */
	}
	if ((_slog_fp = fopen(logname, "a")) == NULL) {
		_slog_fp = stderr;
	}
	slog("SESSION START\n");
	return 0;
}

int slog_close(void)
{
	if (_slog_fp && (_slog_fp != stderr)) {
		fclose(_slog_fp);
	}
	_slog_fp = NULL;
	return 0;
}

int slog(char *fmt, ...)
{
	va_list ap;
	char	tmbuf[64];
	struct	tm	*lctm;
	time_t	sec;
	
	time(&sec);
        lctm = localtime(&sec);
        sprintf(tmbuf, "[%d%02d%02d%02d%02d%02d] ", lctm->tm_year + 1900,
                        lctm->tm_mon + 1, lctm->tm_mday,
                        lctm->tm_hour, lctm->tm_min, lctm->tm_sec);

	if (_slog_fp) {
		fputs(tmbuf, _slog_fp);
		va_start(ap, fmt);
		vfprintf(_slog_fp, fmt, ap);
		va_end(ap);
		fflush(_slog_fp);
	}
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
	return 0;
}


