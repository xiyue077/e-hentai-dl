

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "urltool.h"

#define MAX_URL_LIST	128

typedef	struct	_EHBUF	{
	char	*urlist[MAX_URL_LIST][3];
	int	keysn[32];
	char	buffer[4];
} EHBUF;

static	char	_session_dir[128];

static	int	_img_loaded = 0;
static	int	_img_skipped = 0;
static	char	_tag_update[128];

static int e_hentai_front_url(char *url);
static int e_hentai_front_page(char *webpage);
static int e_hentai_synopsis(char *webpage, char *buf, int len);
static int e_hentai_session_page(char *webpage);
static int e_hentai_session_url(char *url);
static int e_hentai_download(char *urbuf, int *now, int *last);
static int e_hentai_try_orignal(EHBUF *ehbuf, char *fname);
static char *e_hentai_load_webpage(char *url, char *fname, int fnlen);
static int e_hentai_dump_url(EHBUF *ehbuf);
static char *e_hentai_find_url(EHBUF *ehbuf, int cmd);
static EHBUF *e_hentai_url_list(char *webpage);
static void e_hentai_page_stat(EHBUF *ehbuf);
static char *e_hentai_url_filename(char *url, char *buffer, int blen);
//static int e_hentai_url_compare(char *dest, char *sour);
static char *e_hentai_image_name(char *imgurl, char *pname, int last);
static int e_hentai_trap(char *url);
static int e_hentai_is_image_path(char *url);
static int e_hentai_dict_lookup(char *urls[3], char *dict[], int sn, char *id);
static int e_hentai_dict_update(char *s, char *dict1[], char *dict2[], char *prompt);



char	*help_extra = "\
     --help-unit            generic function tests\n\
     --help-url PAGE        list all URLs in the webpage\n\
     --help-dump PAGE       dump important URLs in the webpage\n\
     --help-html PAGE       break down the HTML elements in the webpage\n\
     --help-synopsis PAGE   collect synopsis contents from the front page\n\
     --help-download URL    download one image/page by the session URL\n\
     --help-harvest  URL    continuously download by the session URL\n\
     --help-reload   PAGE   download one image/page by the pre-downloaded session page\n\
     --help-entry PAGE|URL  download one image/page by the front page or URL\n\
     --help-update PAGE ... collecting internal resource name\n\
";


int e_hentai_test(int argc, char **argv)
{
	EHBUF	*ehbuf;
	char	*p, *webpage, key[512];
	int	i, k, rc;

	/* no argument section */
	if (!strcmp(*argv, "--help-unit")) {
		urltool();
		p = e_hentai_image_name("https://iu.sa.ha/h/f5/key=26;xres=1280/04.jpg", "1958342-5", 9);
		printf("e_hentai_image_name: %s\n", p);
		cflags_set(CFLAGS_SORT);
		p = e_hentai_image_name("https://iu.sa.ha/h/f5/key=26;xres=1280/04.jpg", "1958342-5", 19);
		printf("e_hentai_image_name: %s\n", p);
		p = e_hentai_image_name("https://iu.sa.ha/h/f5/key=26;xres=1280/04jpg", "1958342-5", 1999);
		printf("e_hentai_image_name: %s\n", p);
		return 0;
	}

	if (argc < 2) {
		puts(help_extra);
		return 0;
	}
	
	/* one argument section */
	if (!strcmp(*argv, "--help-url")) {
		if ((webpage = htm_alloc(argv[1], 0, NULL)) != NULL) {
			if ((ehbuf = e_hentai_url_list(webpage)) != NULL) {
				e_hentai_find_url(ehbuf, URL_CMD_ALL);
				free(ehbuf);
			}
			free(webpage);
		}
	} else if (!strcmp(*argv, "--help-dump")) {
		if ((webpage = htm_alloc(argv[1], 0, NULL)) != NULL) {
			if ((ehbuf = e_hentai_url_list(webpage)) != NULL) {
				e_hentai_dump_url(ehbuf);
				free(ehbuf);
			}
			free(webpage);
		}
	} else if (!strcmp(*argv, "--help-html")) {
		htm_break(argv[1]);
	} else if (!strcmp(*argv, "--help-synopsis")) {
		if ((webpage = htm_alloc(argv[1], 0, NULL)) != NULL) {
			i = e_hentai_synopsis(webpage, key, sizeof(key));
			printf("%d: %s\n", i, key);
			free(webpage);
		}
	} else if (!strcmp(*argv, "--help-download")) {
		strx_strncpy(key, argv[1], sizeof(key));
		rc = e_hentai_download(key, &i, &k);
		printf("RC=%d %d %d: %s\n", rc, i, k, key);
	} else if (!strcmp(*argv, "--help-harvest")) {
		strx_strncpy(key, argv[1], sizeof(key));
		rc = e_hentai_session_url(key);
	} else if (!strcmp(*argv, "--help-reload")) {
		strx_strncpy(key, argv[1], sizeof(key));
		cflags_clear(CFLAGS_CONTINUOUS);
		rc = e_hentai_session_page(key);
	} else if (!strcmp(*argv, "--help-entry")) {
		strx_strncpy(key, argv[1], sizeof(key));
		cflags_clear(CFLAGS_CONTINUOUS);
		rc = e_hentai_front_url(key);
	} else if (!strcmp(*argv, "--help-update")) {
		for (i = 1; i < argc; i++) {
			if ((webpage = htm_alloc(argv[i], 0, NULL)) != NULL) {
				if ((ehbuf = e_hentai_url_list(webpage)) != NULL) {
					e_hentai_find_url(ehbuf, URL_CMD_FIRST);
					e_hentai_find_url(ehbuf, URL_CMD_LAST);
					e_hentai_find_url(ehbuf, URL_CMD_PREV);
					e_hentai_find_url(ehbuf, URL_CMD_NEXT);
					e_hentai_find_url(ehbuf, URL_CMD_BACK);
					free(ehbuf);
				}
				free(webpage);
			}
		}
	}
	return 0;
}

void e_hentai_statement(int opt)
{
	(void) opt;

	if (_img_loaded || _img_skipped) {
		if (_tag_update[0]) {
			slog("Tag Updated:       %s\n", _tag_update);
		}
		slog("Images Downloaded: %d\n", _img_loaded);
		slog("Images Missed:     %d\n\n", _img_skipped);
	}
}

int e_hentai_process_page(char *webpage, char *fpage)
{
	/* Front Page: https://e-hentai.org/g/2315356/41ccb04708/
	 * <link rel="stylesheet" type="text/css" href="https://e-hentai.org/z/0352/g.css" />
	 * <meta name="description" content="Free Hentai Artist CG Gallery:
	 *
	 * Session Page: https://e-hentai.org/s/bbd6b7c0ac/2315356-1
	 * <link rel="stylesheet" type="text/css" href="https://e-hentai.org/z/0352/g.css" />
	 * </head>
	 * <body>
	 */
	strcpy(_session_dir, ".");	/* always working in the current directory */
	cflags_set(CFLAGS_KEEP_PAGE);	/* always save the webpage */
	if (strstr(webpage, "<meta name=")) {	/* found a front page */
		e_hentai_front_page(webpage);
	} else {
		e_hentai_session_page(webpage);
	}
	return 0;
}

int e_hentai_process_url(char *url)
{
	strcpy(_session_dir, ".");	/* It will be overwritten in e_hentai_front_url() */
	cflags_set(CFLAGS_KEEP_PAGE);	/* always save the webpage */
	if (url_get_index(url, NULL, 0) < 0) {
		/* URL to the front page, such as:
		 * https://e-hentai.org/g/1958342/016a3b968d/ */
		e_hentai_front_url(url);
	} else {
		/* URL to the session page, such as:
		 * https://e-hentai.org/s/7ea07158bd/1958342-5 */
		e_hentai_session_url(url);
	}
	return 0;
}

/* start to rip the image from an URL to the front page, like 
 *   https://e-hentai.org/g/1958342/016a3b968d/ 
 * or the pre-downloaded front page 016a3b968d */
static int e_hentai_front_url(char *url)
{
	char	*webpage, cpage[1024];

	e_hentai_url_filename(url, cpage, sizeof(cpage));
	unlink(cpage);		/* in case the front page exists */
	if (!mkdir(cpage, 0755) || (errno == EEXIST)) {
		chdir(cpage);
		strx_strncpy(_session_dir, cpage, sizeof(_session_dir));
	}
		
	/* download the front page */
	if ((webpage = e_hentai_load_webpage(url, NULL, 0)) != NULL) {
		e_hentai_front_page(webpage);
	}

	if (strcmp(_session_dir, ".")) {
		chdir("..");
	}
	free(webpage);
	return 0;
}

static int e_hentai_front_page(char *webpage)
{
	char	curl[1024];

	switch (e_hentai_synopsis(webpage, curl, sizeof(curl))) {
	case 0:
		e_hentai_session_url(curl);
		break;
	case 1:	/* URL was blocked by content warning so download the page again */
		if ((webpage = e_hentai_load_webpage(curl, NULL, 0)) != NULL) {
			if (!e_hentai_synopsis(webpage, curl, sizeof(curl))) {
				e_hentai_session_url(curl);
			}
			free(webpage);
		}
		break;
	}
	return 0;
}


/* parse the front page like 016a3b968d.
 * Return 0 and the URL to the first session page like 
 *   https://e-hentai.org/s/7dc9685e68/1958342-1
 * Or return 1 and the uncensored URL like
 *   https://e-hentai.org/g/1958342/016a3b968d/?nw=always
 * if there was a content warning. */
static int e_hentai_synopsis(char *webpage, char *buf, int len)
{
	FILE	*fp;
	char	*p, *sp, doc[1024];
	int	n;

	/* create the profile 
	 * <link rel="canonical" href="https://e-hentai.org/g/2315356/41ccb04708/" />*/
	/* 20220913: some pages lost the canonical link. Try these instead:
	 * var token = "01a30953ed";
	 * <a href="https://e-hentai.org/g/1893595/01a30953ed/?report=select">Report Gallery</a>
	 * <a href="https://e-hentai.org/g/1893595/01a30953ed/" onclick="return false">1</a> */
	if ((p = strstr(webpage, "<link rel=\"canonical\"")) != NULL) {
		htm_tag_pick(p, "href=\"", "\"", doc, sizeof(doc));
		p = url_get_tail(doc, '/');
	} else if ((p = strx_strstr(webpage, NULL, "var", ";", "token", "=", NULL)) != NULL) {
		htm_common_pick(p, "\"", "\"", doc, sizeof(doc));
		p = doc;
	} else if ((p = strx_strstr(webpage, NULL, "<a href=", ">", "onclick", "return false", NULL)) != NULL) {
		htm_tag_pick(p, "href=\"", "\"", doc, sizeof(doc));
		p = url_get_tail(doc, '/');
	}
	fp = stdout;
	if (p) {
		strcat(p, ".txt");
		if ((fp = fopen(p, "a+")) == NULL) {
			fp = stdout;
		}
	}
	/*printf("Save to %s\n", p);*/

	/* add the UTF-8 BOM to the log file
	 * https://en.wikipedia.org/wiki/Byte_order_mark 
	 * https://stackoverflow.com/questions/2223882/
	 *    whats-the-difference-between-utf-8-and-utf-8-without-bom */
	/* Pluma largely requires 4-byte in BOM * likely a bug in Pluma *
	 * so added an extra LR which seemingly harmless */
	fputs("\xEF\xBB\xBF\x0A", fp);

	/* <title> [Momoyama Jirou] Ryoujoku Seme - E-Hentai Galleries </title> */
	if (htm_doc_pick(webpage, NULL, "<title", "</title>", doc, sizeof(doc))) {
		fprintf(fp, "Title: %s\n", doc);
	}

	/* <h1 id="gn">Operation Report</h1> */
	p = webpage;
	while ((p = htm_doc_pick(p, NULL, "<h1 id=", "</h1>", doc, sizeof(doc))) != NULL) {
		if (!strx_blankline(doc)) {
			fprintf(fp, "Title: %s\n", doc);
		}
	}

	/* <td class="gdt1">File Size:</td><td class="gdt2">55.20 MB</td> */
	if (strx_strstr(webpage, &p, "<td", "</td>", "File Size:", NULL)) {
		if (htm_doc_pick(p, NULL, "<td", "</td>", doc, sizeof(doc))) {
			fprintf(fp, "File Size: %s\n", doc);
		}
	}

	/* <td class="gdt1">Length:</td><td class="gdt2">12 pages</td> */
	if (strx_strstr(webpage, &p, "<td", "</td>", "Length:", NULL)) {
		if (htm_doc_pick(p, NULL, "<td", "</td>", doc, sizeof(doc))) {
			fprintf(fp, "Length: %s\n", doc);
		}
	}

	/* <td class="gdt1">Language:</td><td class="gdt2">Chinese &nbsp;
	 * <span class="halp" title="This gallery has been translated from the original language text.">
	 * TR</span></td> */
	if (strx_strstr(webpage, &p, "<td", "</td>", "Language:", NULL)) {
		if (htm_doc_pick(p, NULL, "<td", "</td>", doc, sizeof(doc))) {
			if ((p = strstr(doc, "&nbsp;")) != NULL) {
				*p = 0;
			}
			if ((p = strchr(doc, '<')) != NULL) {
				*p = 0;
			}
			fprintf(fp, "Language: %s\n", doc);
		}
	}

	/* <a id="ta_artist:momoyama_jirou" href="https://e-hentai.org/tag/artist:momoyama+jirou" 
	 * class="" onclick="return toggle_tagmenu('artist:momoyama jirou',this)"> momoyama jirou </a> */
	if (htm_doc_pick(webpage, NULL, "<a id=\"ta_artist:", "</a>", doc, sizeof(doc))) {
		fprintf(fp, "Artist: %s\n", doc);
	}

	/* <td class="ptds"><a href="https://e-hentai.org/g/1833366/392f0255bb/" 
	 * onclick="return false">1</a> */
	p = webpage;
	while ((p = htm_doc_pick(p, &sp, "<a href=\"https://e-hentai.org/g/",
			"</a>", doc, sizeof(doc))) != NULL) {
		if (!strcmp(doc, "1")) {
			htm_tag_pick(sp, "<a href=\"", "\"", doc, sizeof(doc));
			fprintf(fp, "HOME: %s\n", doc);
			break;
		}
	}

	/* <a href="https://e-hentai.org/s/1fd021ff4e/1833366-1"><img alt="01" 
	 * title="Page 1: 00.jpg" src="https://ehgt.org/g/blank.gif" 
	 * style="width:100px; height:128px; margin:-1px 0 0 -1px" /></a> */

	/* <p style="text-align:center">
	 * [<a href="https://e-hentai.org/g/1826385/4a848bf134/?nw=session">View Gallery</a>] 
	 * [<a href="https://e-hentai.org/">Get Me Outta Here</a>]</p>
	 * <p style="text-align:center">
	 * [<a href="https://e-hentai.org/g/1826385/4a848bf134/?nw=always">Never Warn Me Again</a>]</p> */
	n = 0;
	if ((p = strstr(webpage, "<a href=\"https://e-hentai.org/s/")) != NULL) {
		htm_tag_pick(p, "\"", "\"", buf, len);
		fprintf(fp, "URL: %s\n", buf);
	} else 	if ((p = strstr(webpage, "Never Warn Me Again")) != NULL) {
		/* URL blocked by content warning */
		for (sp = p; *sp != '<'; sp--);
		htm_tag_pick(sp, "\"", "\"", buf, len);
		fprintf(fp, "Re-URL: %s\n", buf);
		n = 1;
	} else {
		printf("Error: URL not found!\n");
		n = -3;
	}

	if (fp != stdout) {
		fclose(fp);
	}
	return n;
}


/* start to rip the image from a pre-downloaded session page like 1833366-3 
 * because the session page includes a timestamp, the session page must be
 * reloaded to receive a valid URL */  
static int e_hentai_session_page(char *webpage)
{
	EHBUF	*ehbuf;
	char	*url, buf[1024];

	if ((ehbuf = e_hentai_url_list(webpage)) == NULL) {
		return ERR_PARSE;
	}

	/* reload the previous page. */
	if ((url = e_hentai_find_url(ehbuf, URL_CMD_PREV)) == NULL) {
		free(ehbuf);
		return ERR_URL_LAST;
	}
	strx_strncpy(buf, url, sizeof(buf));
	free(ehbuf);

	/* 20220629 downloading under current page's name has a logical fault.
	 * If downloading failed, the current page would be zero-ed and not be able
	 * to reload again. If succeed but reload failed, the current page would
	 * permenantly turned into the previous page */
	/* 20240827 using the 'fnlen' as a flag to tell the current page is the 
	 * front page or the session page; the front page would be saved as .html */
	if((webpage = e_hentai_load_webpage(buf, NULL, 1)) == NULL) {
		return ERR_DL_PAGE;	/* failed to download again */
	}
	if ((ehbuf = e_hentai_url_list(webpage)) == NULL) {
		free(webpage);
		return ERR_PARSE;
	}
	if ((url = e_hentai_find_url(ehbuf, URL_CMD_NEXT)) == NULL) {
		free(ehbuf);
		free(webpage);
		return ERR_URL_NEXT;
	}
	strx_strncpy(buf, url, sizeof(buf));
	free(ehbuf);
	free(webpage);
	return e_hentai_session_url(buf);
}


/* start to rip the image from an URL to the session page, like 
 *   https://e-hentai.org/s/7dc9685e68/1958342-1  */
static int e_hentai_session_url(char *url)
{
	char	curl[1024];
	int	skip, taken, rc, now, last;

	strx_strncpy(curl, url, sizeof(curl));

	skip = taken = now = last = 0;
	while ((rc = e_hentai_download(curl, &now, &last)) > 0) {
		switch (rc) {
		case 1:		/* taken the image and continue */
			taken++;
			break;
		case 2:		/* miss the image link and continue */
			skip++;
			break;
		case 3:		/* download image failed and continue */
			skip++;
			break;
		}
		printf("Waiting %d seconds.\n", sys_delay());
	}
	switch (rc) {
	case 0:		/* received the last image and quit */
		taken++;
		break;
	case ERR_DL_PAGE:
		slog("%s : download page failed %s\n", _session_dir, curl);
		if (last > 0) {
			skip += last - now + 1;
		}
		break;
	case ERR_PARSE:
		slog("%s : parse page failed %s\n", _session_dir, curl);
		if (last > 0) {
			skip += last - now + 1;
		}
		break;
	case ERR_URL_LAST:
		slog("%s : could not find the last URL %s\n", _session_dir, curl);
		if (last > 0) {
			skip += last - now + 1;
		}
		break;
	case ERR_URL_NEXT:
		slog("%s : could not find the next URL %s\n", _session_dir, curl);
		if (last > 0) {
			skip += last - now + 1;
		}
		break;
	case ERR_TRAP:
		slog("%s : TRAP %s\n", _session_dir, curl);
		if (last > 0) {
			skip += last - now + 1;
		}
		break;
	case ERR_DL_IMAGE:
		//slog("%s : missed the last image %s\n", _session_dir, curl);
		skip++;
		break;
	}
	slog("%s : %d downloaded; %d missed\n", _session_dir, taken, skip);
	_img_loaded += taken;
	_img_skipped += skip;
	return rc;
}


/* download the session page and the image from the session URL like 
 *   https://e-hentai.org/s/7dc9685e68/1958342-1  
 * when succeed, the next session URL will be returned in 'urbuf'  */
static int e_hentai_download(char *urbuf, int *now, int *last)
{
	EHBUF	*ehbuf;
	char	*url, *webpage, *fname, cpage[64];
	int	rc;

	*now = url_get_index(urbuf, NULL, 0);

	/* download the session page */
	if ((webpage = e_hentai_load_webpage(urbuf, cpage, sizeof(cpage))) == NULL) {
		slog("Download from %s failed\n", urbuf);
		return ERR_DL_PAGE;	/* download the session page failed */
	}

	/* ananlysis the session page */
	if ((ehbuf = e_hentai_url_list(webpage)) == NULL) {
		free(webpage);
		return ERR_PARSE;	/* parse the session page failed */
	}

	e_hentai_dump_url(ehbuf);

	/* find the last page */
	if ((url = e_hentai_find_url(ehbuf, URL_CMD_LAST)) == NULL) {
		free(ehbuf);
		free(webpage);
		return ERR_URL_LAST;	/* parsing failed: missing the last URL */
	}
	if (*last == 0) {
		*last = url_get_index(url, NULL, 0);
	} else if (*last != url_get_index(url, NULL, 0)) {
		free(ehbuf);
		free(webpage);
		return ERR_URL_LAST;	/* last page should never change */
	}

	/* download the embeded image */
	rc = 0;
	if (cflags_check(CFLAGS_MEDIA)) {
		if ((url = e_hentai_find_url(ehbuf, URL_CMD_IMAGE)) == NULL) {
			slog("%s : not found the image link.\n", cpage);
			rc = 2;
		} else {
			fname = e_hentai_image_name(url, cpage, *last);
			if (e_hentai_try_orignal(ehbuf, fname) != 0) {
				if (sys_download_wget_image(url, fname) != 0) {
					slog("%s : failed to get %s\n", cpage, 
							e_hentai_url_filename(url, NULL, 0));
					rc = 3;
				}
			}
		}
	}

	if (!cflags_check(CFLAGS_CONTINUOUS)) {
		free(ehbuf);
		free(webpage);
		return rc ? ERR_DL_IMAGE : 0;	/* just download one */
	}

	/* check if this is the last page */
	if (*now == *last) {
		free(ehbuf);
		free(webpage);
		return rc ? ERR_DL_IMAGE : 0;	/* reach the last page */
	}

	/* move to the next page */
	if ((url = e_hentai_find_url(ehbuf, URL_CMD_NEXT)) == NULL) {
		free(ehbuf);
		free(webpage);
		return ERR_URL_NEXT;	/* parsing failed: missing the next URL */
	}
	if (e_hentai_trap(url)) {	/* it maybe a trap */
		free(ehbuf);
		free(webpage);
		return ERR_TRAP;	/* parsing failed: maybe a trap */
	}

	strcpy(urbuf, url);
	free(ehbuf);
	free(webpage);
	return rc ? rc : 1;	/* move to next URL */
}

static int e_hentai_try_orignal(EHBUF *ehbuf, char *fname)
{
	char	*url;
	int	rc;

	if ((url = e_hentai_find_url(ehbuf, URL_CMD_ORIGIN)) == NULL) {
		return 1;	/* not available */
	}
	if (sys_download_cookies_open(NULL) == 0) {
		return 2;	/* require cookies */
	}

	/* just try once, no retry if failed
	 * only turn on the cookies while downloading the picture */
	cflags_set(CFLAGS_COOKIE);
	rc = sys_download_wget(url, fname);
	cflags_clear(CFLAGS_COOKIE);
	return rc;
}

/* 20240827 using the 'fnlen' as a flag to tell the current page is the front
 * page or the session page; the front page would be saved with a suffix. 
 * So when calling this function: 
 *  fname==NULL, fnlen==0: the front page like e5977d80be saved as e5977d80be.html
 *  fname==NULL, fnlen!=0: the session page lile 313773-1
 *  fname!=NULL, fnlen!=0: the session page lile 313773-2
 * */
static char *e_hentai_load_webpage(char *url, char *fname, int fnlen)
{
	char	localfile[1024];

	e_hentai_url_filename(url, localfile, sizeof(localfile));
	if (fname) {
		strx_strncpy(fname, localfile, fnlen);
	} else if (fnlen == 0) {
		strcat(localfile, ".html");
	}

	if (sys_download_wget_page(url, localfile) != 0) {
		slog("%s : failed to retrieve the web page.\n", localfile);
		return NULL;
	}
	return htm_alloc(localfile, 0, NULL);
}

static int e_hentai_dump_url(EHBUF *ehbuf)
{
	char	*url;

	if (cflags_check(CFLAGS_DUMP)) {
		printf("Page: %d %d %d %d\n", 
				ehbuf->keysn[0], ehbuf->keysn[1], 
				ehbuf->keysn[2], ehbuf->keysn[3]);
		printf("First Page: %s\n", 
				e_hentai_find_url(ehbuf, URL_CMD_FIRST));
		printf("Last Page:  %s\n", 
				e_hentai_find_url(ehbuf, URL_CMD_LAST));
		printf("Prev Page:  %s\n", 
				e_hentai_find_url(ehbuf, URL_CMD_PREV));
		url = e_hentai_find_url(ehbuf, URL_CMD_NEXT);
		printf("Next Page:  %s (%d)\n", 
				url, e_hentai_trap(url));
		printf("Front Page: %s\n", 
				e_hentai_find_url(ehbuf, URL_CMD_BACK));
		printf("Original:   %s\n",
				e_hentai_find_url(ehbuf, URL_CMD_ORIGIN));
		printf("Image URL:  %s\n\n", 
				e_hentai_find_url(ehbuf, URL_CMD_IMAGE));
	}
	if (cflags_check(CFLAGS_DUMP_ALL)) {
		e_hentai_find_url(ehbuf, URL_CMD_ALL);	
	}
	if (cflags_check(CFLAGS_DUMP_MEDIA)) {
		url = e_hentai_find_url(ehbuf, URL_CMD_IMAGE);
		printf("%s\n", e_hentai_url_filename(url, NULL, 0));
	}
	return 0;
}

static char *e_hentai_find_url(EHBUF *ehbuf, int cmd)
{
	char	*dict_first[] = 
		{ "f.png", "p.afk", "M.iha", "7.qqm", "f.lol", "Q.bbq", 
		  "a.tlc", "o.ffs", NULL };
	char	*dict_last[]  = 
		{ "l.png", "O.ffs", "T.afk", "d.lol", "x.qqm", "l.iha", 
		  "h.bbq", "Z.tlc", NULL };
	char	*dict_prev[] = 
		{ "p.png", "prev.png", NULL };
	char	*dict_next[]  = 
		{ "n.png", "S.bbq", "b.tlc", "P.afk", "F.lol", "W.ffs", 
		  "g.qqm", "e.iha", "next.png", NULL };
	char	*dict_back[] = 
		{ "b.png", NULL };
	int	i;

	for (i = 0; ehbuf->urlist[i][0]; i++) {
		if (ehbuf->urlist[i][1] == NULL) {
			continue;
		}

		switch (cmd) {
		case URL_CMD_FIRST:
			if (e_hentai_dict_lookup(ehbuf->urlist[i], dict_first, ehbuf->keysn[0], NULL)) {
				e_hentai_dict_update(ehbuf->urlist[i][1], dict_first, dict_prev, "FIRST");
				return ehbuf->urlist[i][0];
			}
			break;
		case URL_CMD_LAST:
			if (e_hentai_dict_lookup(ehbuf->urlist[i], dict_last, ehbuf->keysn[3], NULL)) {
				e_hentai_dict_update(ehbuf->urlist[i][1], dict_last, dict_next, "LAST");
				return ehbuf->urlist[i][0];
			}
			break;
		case URL_CMD_NEXT:
			if (e_hentai_dict_lookup(ehbuf->urlist[i], dict_next, ehbuf->keysn[2], "next")) {
				e_hentai_dict_update(ehbuf->urlist[i][1], dict_next, dict_last, "NEXT");
				return ehbuf->urlist[i][0];
			}
			break;
		case URL_CMD_PREV:
			if (e_hentai_dict_lookup(ehbuf->urlist[i], dict_prev, ehbuf->keysn[1], "prev")) {
				e_hentai_dict_update(ehbuf->urlist[i][1], dict_prev, dict_first, "PREV");
				return ehbuf->urlist[i][0];
			}
			break;
		case URL_CMD_BACK:
			if (e_hentai_dict_lookup(ehbuf->urlist[i], dict_back, -1, "back")) {
				e_hentai_dict_update(ehbuf->urlist[i][1], dict_back, dict_back, "BACK");
				return ehbuf->urlist[i][0];
			}
			break;
		case URL_CMD_ORIGIN:
			if ((ehbuf->urlist[i][0] == ehbuf->urlist[i][1]) && !ehbuf->urlist[i][2]) {
				return ehbuf->urlist[i][0];
			}
			break;
		case URL_CMD_IMAGE:
			//printf("%s | %s\n", (ehbuf->urlist[i][0]), (ehbuf->urlist[i][1]));
			if (e_hentai_dict_lookup(ehbuf->urlist[i], dict_next, -1, "next")) {
				break;	/* image url has the same serial number, so -1 here */
			}
			if (e_hentai_dict_lookup(ehbuf->urlist[i], dict_first, ehbuf->keysn[0], NULL)) {
				break;
			}
			if (e_hentai_dict_lookup(ehbuf->urlist[i], dict_last, -1, NULL)) {
				break;
			}
			if (e_hentai_dict_lookup(ehbuf->urlist[i], dict_prev, ehbuf->keysn[1], "prev")) {
				break;
			}
			if (url_get_index(ehbuf->urlist[i][0], NULL, 0) != ehbuf->keysn[2]) {
				break;
			}
			if (!url_is_image(ehbuf->urlist[i][1])) {
				break;
			}
			if (!e_hentai_is_image_path(ehbuf->urlist[i][1])) {
				break;
			}
			if (cmd == URL_CMD_NEXT) {
				return ehbuf->urlist[i][0];
			} else {
				return ehbuf->urlist[i][1];
			}
		default:	/* URL_CMD_ALL */
			puts(ehbuf->urlist[i][0]);
			if (ehbuf->urlist[i][2]) {
				printf("----> %s (%s)\n", 
						ehbuf->urlist[i][1], ehbuf->urlist[i][2]);
			} else {
				printf("----> %s\n", ehbuf->urlist[i][1]);
			}
			break;
		}
	}
	return NULL;
}

static EHBUF *e_hentai_url_list(char *webpage)
{
	EHBUF	*ehbuf;
	char	*p, *q;
	int	n;

	if ((ehbuf = calloc(1, sizeof(EHBUF))) == NULL) {
		return NULL;
	}

	p = webpage;
	n = 0;
	/* 20130905 ehentai seems updated their webpages */
	/*while ((p = strstr(p, "a href=\"")) != NULL) {
		p += 8; */
	while ((p = strstr(p, "href=\"")) != NULL) {
		/* find the 'id' tag */
		for (q = p; *q != '<'; q--);
		//if (!strx_strncmp(q, "<a id=\"")) {
		ehbuf->urlist[n][2] = htm_tag_pick(q, "id=\"", "\"", NULL, -1);
		//}

		/* find the content of 'href' */
		ehbuf->urlist[n][0] = htm_tag_pick(p, "href=\"", "\"", NULL, -1);
		p += strlen(p) + 1;

		if (!strx_strncmp(p, "><img src=\"")) {
			ehbuf->urlist[n][1] = htm_tag_pick(p, "><img src=\"", "\"", NULL, -1);
			p += strlen(p) + 1;
			n++;
			if (n >= MAX_URL_LIST) {
				break;
			}
		}
		/* 20130905 ehentai seems updated their webpages
		 *   <a onclick="return load_image(148,'144e04abd9')" href="https://e-hentai.org/s/144e04abd9/
		 *   2395276-148"><img id="img" src="https://fzqcalj.czofmljnxqaw.hath.network:7777/h/
		 *   45450cbf5114247adcb27eeec04f3bfed54bcb62-127691-1280-720-jpg/keystamp=1675099200-
		 *   ad9f6698cf;fileindex=117627751;xres=1280/0147.jpg" style="height:720px;width:1280px" 
		 *   onerror="this.onerror=null; nl('40866-465305')" /></a> */
		else if (!strx_strncmp(p, "><img id=\"img\" src=\"")) {
			ehbuf->urlist[n][1] = htm_tag_pick(p, "><img id=\"img\" src=\"", "\"", NULL, -1);
			p += strlen(p) + 1;
			n++;
			if (n >= MAX_URL_LIST) {
				break;
			}
		}
		/* 20230130 Download original
		 * Some session page includes a link to the original picture, which 
		 * usually larger than the display picture. Note that accessing the 
		 * original picture requiring login via cookies.
		 *   <div id="i7" class="if"> &nbsp;<img src="https://ehgt.org/g/mr.gif" class="mr" />
		 *   <a href="https://e-hentai.org/fullimg.php?gid=2449394&amp;page=1&amp;key=oi9a1v39z04">
		 *   Download original 1280 x 2048 3.21 MB source</a></div>
		 */
		else if (!strx_strncmp(p, ">Download original")) {
			/* In that case, there's no "<a id=" so ehbuf->urlist[n][2] is NULL
			 * ehbuf->urlist[n][0] is the image url */
			ehbuf->urlist[n][1] = ehbuf->urlist[n][0];
			n++;
			if (n >= MAX_URL_LIST) {
				break;
			}
		}
	}

	e_hentai_page_stat(ehbuf);
	return ehbuf;
}

static void e_hentai_page_stat(EHBUF *ehbuf)
{
	char	*keystr[4], *s;
	int	i, k, n, rc;

	for (i = 0; i < 4; i++) {
		keystr[i] = NULL;
		ehbuf->keysn[i] = 0;
	}
	for (i = n = 0; ehbuf->urlist[i][0]; i++) {
		if (ehbuf->urlist[i][1] == NULL) {
			continue;
		}
		if ((rc = url_get_index(ehbuf->urlist[i][0], NULL, 0)) < 0) {
			continue;
		}
		/* quick fix to this type:
		 * http://g.e-hentai.org/s/72l037b429/436259-135
		 * ----> http://g.ehgt.org/img/n/next.png
		 *  http://g.e-hentai.org/s/3ff0e48c9a/436259-1
		 *  ----> http://g.ehgt.org/img/f.png
		 *  http://g.e-hentai.org/s/dcf6300cdd/436259-133
		 *  ----> http://g.ehgt.org/img/p.png
		 *  http://g.e-hentai.org/s/721037b429/436259-135
		 *  ----> http://g.ehgt.org/img/n.png
		 *  ...  */
		if ((n == 0) && (rc > 4)) {
		       continue;
		}	       
		s = url_get_tail(ehbuf->urlist[i][1], '/');
		for (k = 0; k < n; k++) {
			if (!strcmp(keystr[k], s)) {
				break;
			}
		}
		if (k == n) {
			keystr[k] = s;
			ehbuf->keysn[k] = rc;
			n++;
			if (n == 4) {
				break;
			}
		}
	}
	for (i = 0; i < 3; i++) {
		for (k = i + 1; k < 4; k++) {
			if (ehbuf->keysn[i] > ehbuf->keysn[k]) {
				rc = ehbuf->keysn[i];
				ehbuf->keysn[i] = ehbuf->keysn[k];
				ehbuf->keysn[k] = rc;
			}
		}
	}
}

/* http://50.7.233.114/ehg/image.php?f=5154150da59ceb92e8ce9fd13ef7bab7615cd
 *   b58-503119-892-1237-png&amp;t=370249-94283429070c8dadfba302cdbdd75f3dfb
 *   dedc12&amp;n=039.png
 * http://80.222.98.188:5362/h/f4379bf970692e51f925ff40b1957171f60f37b2-5261
 *   65-1053-1500-jpg/keystamp=1332992557-7151651d63/otakumatsuri8_28.jpg
 * https://e-hentai.org/g/725785/5bb14fb5d4/?nw=always
*/
static char *e_hentai_url_filename(char *url, char *buffer, int blen)
{
	char	*p;

	if (strstr(url, "image.php")) {
		p = url_get_tail(url, '=');
	} else {
		p = url_get_tail(url, '/');
		if (*p == '?') {
			p -= 2;
			while ((p > url) && (*p != '/')) p--;
			if (*p == '/') p++;
		}
	}
	if (buffer) {
		strx_strncpy(buffer, p, blen);
		if ((p = strstr(buffer, "/?")) != NULL) {
			*p = 0;
		}
		p = buffer;
	}
	return p;
}


/*
static int e_hentai_url_compare(char *dest, char *sour)
{
	char	*p, *k;

	p = e_hentai_url_filename(dest, NULL, 0);
	k = e_hentai_url_filename(sour, NULL, 0);
	return strcmp(p, k);
}
*/

static char *e_hentai_image_name(char *imgurl, char *pname, int last)
{
	static	char	buffer[1024];
	FILE	*fp;
	char	key[1000];
	int	n;
	//char	*tail;

	buffer[0] = 0;
	if (cflags_check(CFLAGS_SORT)) {
		/* such as 1958342-5, key[] == 1958342, n == 5 */
		n = url_get_index(pname, key, sizeof(key));
		strcat(buffer, url_image_sn(n, last));
		strcat(buffer, "_");
	}

	strcat(buffer, e_hentai_url_filename(imgurl, NULL, 0));

#if 1	/* 20220914 just delete the previous downloaded/broken image */
	if ((fp = fopen(buffer, "r")) != NULL) {
		fclose(fp);
		unlink(buffer);
	}

#else	/* disparse the filename if met the same name */
	if ((tail = strrchr(buffer, '.')) == NULL) {
		key[0] = 0;
		tail = buffer + strlen(buffer);
	} else {
		strx_strncpy(key, tail, sizeof(key));
		*tail = 0;
	}

	if (!cflags_check(CFLAGS_CONTINUOUS)) {		/* just download one */
		if ((fp = fopen(buffer, "r")) != NULL) {
			fclose(fp);
			unlink(buffer);
		}
		return buffer;
	}

	for (n = 0; n < 100; n++) {
		strcat(tail, key);

		if ((fp = fopen(buffer, "r")) == NULL) {
			break;
		}
		fclose(fp);

		sprintf(tail, "_%d", n);
	}
#endif
	return buffer;
}

static int e_hentai_trap(char *url)
{
	char	*p, tmp[512];

	if (url == NULL) {
		return 0;
	}

	strx_strncpy(tmp, url, 512);

	p = url_get_tail(tmp, '/');
	p--;
	*p = 0;
	p = url_get_tail(tmp, '/');
	//puts(p);

	for ( ; *p; p++) {
		if (!isxdigit(*p)) {
			return 1;
		}
	}
	return 0;
}

/* make sure the URL points to the e-hentai's image database, like
 *   https://eqhvbiu.svwfqxcjodba.hath.network/h/f5fd86c282a183b1cbed817f2acd6e000b3dcd93-
 *   477368-1280-1791-jpg/keystamp=1626269700-f962736502;fileindex=94992256;xres=1280/04.jpg
 * not
 *   https://e-hentai.org/r/e1fb99c1ceb817f9843ae4536b13025553e69b6e-340368-1280-1639-jpg
 *   /forumtoken/1833366-1/00.jpg */
/* 20231218: 
 *   https://ziwgdfwwictwelwogupv.hath.network/om/3394274/54d8dd01fe6201d0c18fc0c1cbc895b2cc94cec7-102543-619-393-jpg/x/0/126bfeqtgy7zmw17vi4/1162416289532.jpg
 */
static int e_hentai_is_image_path(char *url)
{
	char	tmp[32];

	if (url_get_path(url, 1, tmp, sizeof(tmp)) == NULL) {
		return 0;
	}
	if (!strcmp(tmp, "h")) {
		return 1;
	}
	if (!strcmp(tmp, "ehg")) {
		return 1;
	}
	if (!strcmp(tmp, "lid")) {
		return 1;
	}
	if (!strcmp(tmp, "im")) {
		return 1;
	}
	if (!strcmp(tmp, "om")) {
		return 1;
	}
	return 0;
}

static int e_hentai_dict_lookup(char *urls[3], char *dict[], int sn, char *id)
{
	char	*fname;
	int	i;

	fname = url_get_tail(urls[1], '/');
	for (i = 0; dict[i]; i++) {
		//printf("%s --> %s\n", fname, dict[i]);
		if (!strcmp(fname, dict[i])) {
			return 1;
		}
	}

	if (urls[2] && id && !strcmp(urls[2], id)) {
		return 1;
	}

	if (url_get_index(urls[0], NULL, 0) == sn) {
		return 1;
	}	
	return 0;
}

static int e_hentai_dict_update(char *s, char *dict1[], char *dict2[], char *prompt)
{
	char	*fname;
	int	i;

	fname = url_get_tail(s, '/');
	for (i = 0; dict1[i]; i++) {
		if (!strcmp(fname, dict1[i])) {
			return 0;
		}
	}
	for (i = 0; dict2[i]; i++) {
		if (!strcmp(fname, dict2[i])) {
			return 0;
		}
	}
	slog("DICT UPDATE: %s [%s]\n", prompt, fname);
	strx_strncpy(_tag_update, fname, sizeof(_tag_update));
	return 1;
}


