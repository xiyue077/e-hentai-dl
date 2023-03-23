
/* 
#!/bin/bash

for i in {1..20}
do
  tmp=$(printf "http://hentai.desi/imglink/manga/19/0000019294/%03d.jpg" $i)
  wget $tmp
  sleep 1
done

*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include "urltool.h"

#define VERSION		"3.2"
#define DEF_FLAGS	(CFLAGS_DUMP | CFLAGS_MEDIA | CFLAGS_CONTINUOUS | CFLAGS_SORT)

int e_hentai_test(int argc, char **argv);
void e_hentai_statement(int opt);
int e_hentai_process_page(char *webpage, char *fpage);
int e_hentai_process_url(char *url);

int hentaiera_test(int argc, char **argv);
void hentaiera_statement(int opt);
int hentaiera_process_page(char *webpage, char *fpage);
int hentaiera_process_url(char *url);

int comicporn_test(int argc, char **argv);
void comicporn_statement(int opt);
int comicporn_process_page(char *webpage, char *fpage);
int comicporn_process_url(char *url);

int motherless_process_page(char *webpage, char *fpage);
int motherless_process_url(char *url);

int heavy_r_process_page(char *webpage, char *fpage);
int heavy_r_process_url(char *url);

int xvideos_process_page(char *webpage, char *fpage);
int xvideos_process_url(char *url);


struct	_extractor	{
	char	*pattern_url;		/* identitier in URL */
	char	*pattern_webpage;	/* identitier in the webpage */

	int	(*unit_test)(int argc, char **argv);
	void	(*statement)(int opt);
	int	(*process_page)(char *webpage, char *fpage);
	int	(*process_url)(char *url);
} extractor[] = {
	{	"e-hentai.org", "e-hentai.org",
		e_hentai_test, e_hentai_statement, e_hentai_process_page, e_hentai_process_url },
	{	"hentaiera.com", NULL,
		hentaiera_test, hentaiera_statement, hentaiera_process_page, hentaiera_process_url },
	{	"comicporn.xxx", NULL,
		comicporn_test, comicporn_statement, comicporn_process_page, comicporn_process_url },
	{	"motherless.com", "motherless.com",
		NULL, NULL, motherless_process_page, motherless_process_url },
	{	"heavy-r.com", "www.heavy-r.com",
		NULL, NULL, heavy_r_process_page, heavy_r_process_url },
	{	"xvideos.com", "www.xvideos.com", 
		NULL, NULL, xvideos_process_page, xvideos_process_url },
	{ NULL, NULL, NULL, NULL }
};

static int dispatch_by_unit_test(int argc, char **argv);
static void dispatch_by_statement(int opt);
static int dispatch_by_url_list(char *fname);
static int dispatch_by_url(char *urlraw);
static int dispatch_by_page_file(char *fpage);
static void dispatch_by_statement(int opt);
static char *e_hentai_make_folder(char *fname);
static int e_hentai_cleanup(char *fname);


char	*help = "\
Usage: e-hentai-dl [-d][-s][-p] [html_page|URL]\n\
  -s,--single        download one image only\n\
  -k,--keep-webpage  save the webpage for further study\n\
  -u,--unsort        do not prefix the sorting number to images\n\
  -d[a|i]            dump URL of [all|image] in the page\n\
  -p,--proxy URL     specify a proxy server\n\
  -o,--cookie FILE   specify a cookie file for wget\n\
     --help          help and more helps by '-'\n\
     --version\n\
\n\
Usage: e-hentai-dl [-c] directory [...]\n\
  -c,--cleanup       clean up the downloaded directories\n\
";


int main(int argc, char **argv)
{
	cflags_set(DEF_FLAGS);
	while (--argc && (**++argv == '-')) {
		if (!strcmp(*argv, "--help")) {
			puts(help);
			return 0;
		} else if (!strcmp(*argv, "--version")) {
			printf("Version %s\n", VERSION);
			return 0;
		} else if (!strx_strncmp(*argv, "--help-")) {
			return dispatch_by_unit_test(argc, argv);
		} else if (!strcmp(*argv, "-s") || !strcmp(*argv,"--single")) {
			cflags_argvs('s');
		} else if (!strcmp(*argv, "-k") || !strcmp(*argv,"--keep-webpage")) {
			cflags_argvs('k');
		} else if (!strx_strncmp(*argv, "-d")) {
			cflags_argvs(argv[0][2]);
		} else if (!strcmp(*argv, "-u") || !strcmp(*argv,"--unsort")) {
			cflags_argvs('u');
		} else if (!strcmp(*argv, "-p") || !strcmp(*argv, "--proxy")) {
			if (--argc < 1) {
				printf("Missing options!\n");
				return -1;
			}
			sys_download_proxy_open(*++argv);
		} else if (!strcmp(*argv, "-o") || !strcmp(*argv, "--cookie")) {
			if (--argc < 1) {
				printf("Missing options!\n");
				return -1;
			}
			sys_download_cookies_open(*++argv);
		} else if (!strcmp(*argv, "-c") || !strcmp(*argv, "--cleanup")) {
			while (--argc) {
				e_hentai_cleanup(*++argv);
			}
			return 0;
		} else {
			printf("Invalided option [%s]\n", *argv);
			return -1;
		}
	}
	if (argc == 0) {
		puts(help);
		return -1;
	}

	if (cflags_check(CFLAGS_MEDIA)) {
		slog_open("e-hentai.log");	/* open the system log */
	}
	//cflags_set(CFLAGS_KEEP_PAGE);
	while (argc--) {
		if (url_is_remote(*argv))  {
			dispatch_by_url(*argv);
		} else { /* given the local file, could be webpage or url list */
			dispatch_by_url_list(*argv);
		}
		argv++;
	}

	dispatch_by_statement(0);
	slog_close();	/* close the system log */
	sys_download_proxy_close();
	return 0;
}

static int dispatch_by_unit_test(int argc, char **argv)
{
	int	i;

	for (i = 0; extractor[i].pattern_url; i++) {
		if (extractor[i].unit_test) {
			return extractor[i].unit_test(argc, argv);
		}
	}
	return 0;
}

static void dispatch_by_statement(int opt)
{
	int	i;

	for (i = 0; extractor[i].pattern_url; i++) {
		if (extractor[i].statement) {
			return extractor[i].statement(opt);
		}
	}
}

static int dispatch_by_url_list(char *fname)
{
	FILE	*fp;
	char	buf[1024];

	if ((fp = fopen(fname, "r")) == NULL) {
		perror(fname);
		return -1;
	}
	while (fgets(buf, sizeof(buf)-1, fp) != NULL) {
		buf[strlen(buf)-1] = 0;
		if (!strncasecmp(buf, "<!DOCTYPE", 9)) {	/* It's a html file */
			fclose(fp);
			return dispatch_by_page_file(fname);
		}

		if (url_is_remote(buf))  {
			dispatch_by_url(buf);
		}
	}
	fclose(fp);
	return 0;
}

static int dispatch_by_url(char *urlraw)
{
	char	*url;
	int	i, rc = ERR_URL_NONE;

	if ((url = strx_alloc(urlraw, 0)) == NULL) {
		return ERR_MEMORY;
	}

	for (i = 0; extractor[i].pattern_url; i++) {
		if (strstr(url, extractor[i].pattern_url)) {
			if (extractor[i].process_url) {
				rc = extractor[i].process_url(url);
				break;
			}
		}
	}
	if (!extractor[i].pattern_url) {
		/* no extractor for this url, try youtube-dl */
		rc = sys_download_ytdl(url);
	}
	free(url);
	return rc;
}

static int dispatch_by_page_file(char *fpage)
{
	char	*webpage, *endp;
	int	i, rc = ERR_URL_NONE;

	if ((webpage = htm_alloc(fpage, 0, NULL)) == NULL) {
		return ERR_MEMORY;
	}

	for (i = 0; extractor[i].pattern_url; i++) {
		if (extractor[i].pattern_webpage) {
			endp = strstr(webpage, extractor[i].pattern_webpage);
		} else {
			endp = strstr(webpage, extractor[i].pattern_url);
		}
		if (endp && extractor[i].process_page) {
			rc = extractor[i].process_page(webpage, fpage);
			break;
		}
	}
	free(webpage);
	return rc;
}


/* pdftohtml -i -stdout 3d-SingleArt.pdf > standard.htm
 * pdfimages -all 3d-SingleArt.pdf standard
 */
static int dispatch_by_pdf(char *pdffile)
{
}

static char *e_hentai_make_folder(char *fname)
{
	FILE	*fp;
	char	author[128], title[256], buf[256];
	static	char	result[512];

	if ((fp = fopen(fname, "r")) == NULL) {
		return NULL;
	}
	author[0] = title[0] = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		strx_strrpch(buf, 1, '\n', 0);		/* chop the tailing '\n' */
		if (!strx_strncmp(buf, "Title: ")) {
			strx_strncpy(title, buf + 7, sizeof(title));
			continue;
		}
		if (!strx_strncmp(buf, "Artist: ")) {
			strx_strncpy(author,  buf + 8, sizeof(author));
			continue;
		}
	}
	fclose(fp);

	if (!title[0]) {
		return NULL;
	} else if (!author[0]) {
		strcpy(result, title);
	} else if (!strncasecmp(author, title + 1, strlen(author))) {
		strcpy(result, title);
	} else {
		sprintf(result, "[%s] %s", author, title);
	}

	/* sanity check */
	strx_strrpch(result, 0, '/', '_');
	strx_strrpch(result, 0, ':', '_');
	strx_strrpch(result, 0, '$', '_');
	strx_strrpch(result, 0, '^', '_');
	strx_strrpch(result, 0, '&', '_');
	return result;
}

static int e_hentai_cleanup(char *fname)
{
        struct	dirent	*de;
	DIR	*dir;
	char	folder[128], cwd[PATH_MAX], *endp, *newname = NULL;
	int	flen;

	getcwd(cwd, sizeof(cwd));
	if (chdir(fname) < 0) {
		perror(fname);
		return 0;
	}

	/* extract the directory name and remove the trailing '/' */
	strx_strncpy(folder, url_get_tail(fname, '/'), sizeof(folder));
	flen = strlen(folder);
	//printf("+ %s\n", folder);
	
	dir = opendir(".");
	while ((de = readdir(dir)) != NULL)  {
		/* using the synopsis of the folder to create a readable 
		 * folder name */
		if (!strncmp(de->d_name, folder, flen) && 
				!strcmp(de->d_name + flen, ".txt")) {
			newname = e_hentai_make_folder(de->d_name);
			continue;
		}

		/* rule out anything with extension names: ., .., *.jpg */ 
		if (strchr(de->d_name, '.')) {
			continue;
		}

		/* rule out anything without '-' */
		if (!strchr(de->d_name, '-')) {
			continue;
		}

		/* the session page must be all number composed, like 2245891-10 */
		strtol(de->d_name, &endp, 10);
		if (*endp != '-') {
			continue;
		}
		strtol(endp+1, &endp, 10);
		if (*endp != 0) {
			continue;
                }

		/* delete all others */
		//printf("delete: %s\n", de->d_name);
		unlink(de->d_name);
	}
	closedir(dir);

	if (newname) {
		chdir("..");
		printf("rename: %s  ->  %s\n", folder, newname);
		rename(folder, newname);
	}
	chdir(cwd);
	return 0;
}

