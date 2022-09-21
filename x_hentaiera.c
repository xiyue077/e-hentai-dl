
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "urltool.h"

#define URL_MAX		32
static	char	_session_dir[256];
static  int     _img_loaded = 0;
static  int     _img_skipped = 0;


static int hentaiera_front_url(char *url);
static int hentaiera_session_url(char *url);
static int hentaiera_front_page(char *webpage, char *fpage);
static int hentaiera_session_page(char *webpage, char *fpage);
static int hentaiera_session_attr(char *webpage, int *curr, int *last);
static char *hentaiera_image_name(char *imgurl, int curr, int last);


int hentaiera_test(int argc, char **argv)
{
	return 0;
}

void hentaiera_statement(int opt)
{
	(void) opt;

	if (_img_loaded || _img_skipped) {
		slog("Images Downloaded: %d\n", _img_loaded);
		slog("Images Missed:     %d\n\n", _img_skipped);
	}
}

/* front page:
 * <title> VACBED | KITSUNE | FETISH ABYSS - HentaiEra </title>
 * session page:
 * <title> VACBED | KITSUNE | FETISH ABYSS - Page 1 - HentaiEra </title>
 */
int hentaiera_process_page(char *webpage, char *fpage)
{
	char	*p;
	int	rc;

        //cflags_set(CFLAGS_KEEP_PAGE);	/* always save the webpage */
	if ((p = strstr(webpage, "<title")) == NULL) {
		return ERR_PARSE;
	}
	if (strx_strstr(p, NULL, "-", "</title>", "Page", NULL)) {
		rc = hentaiera_session_page(webpage, fpage);
		switch (rc) {
		case ERR_NONE:		/* successfully get the image and next url */
			return hentaiera_session_url(webpage);
		case ERR_DL_IMAGE:	/* failed to get the image but get the next url */
			return hentaiera_session_url(webpage);
		default:
			return rc;
		}
	} else {
		rc = hentaiera_front_page(webpage, fpage);
		switch (rc) {
		case ERR_NONE:
			return hentaiera_session_url(webpage);
		default:
			return rc;
		}
	}
	return ERR_NONE;
}

/* front page:
 * https://hentaiera.com/gallery/912729/
 * session page:
 * https://hentaiera.com/view/912729/1/
 */
int hentaiera_process_url(char *url)
{
	strcpy(_session_dir, ".");	/* It will be overwritten in hentaiera_front_url() */
        //cflags_set(CFLAGS_KEEP_PAGE);	/* always save the webpage */
	if (strstr(url, "gallery")) {
		/* URL to the front page, such as:
		 * https://hentaiera.com/gallery/912729/  */
		hentaiera_front_url(url);
        } else {
		/* URL to the session page, such as:
		 * https://hentaiera.com/view/912729/1/  */
		hentaiera_session_url(url);
	}
	return 0;

}

static int hentaiera_front_url(char *url)
{
	char	*webpage, fname[1024];
	int	rc;

	strx_strncpy(fname, url_get_tail(url, '/'), sizeof(fname));
	//printf("%s: %s\n", fname, url);
	unlink(fname);		/* in case the front page does exist */
	if (!mkdir(fname, 0755) || (errno == EEXIST)) {
		chdir(fname);
		strx_strncpy(_session_dir, fname, sizeof(_session_dir));
	}

	if (sys_download_wget_page(url, fname) < 0) {
		slog("%s : failed to retrieve the web page.\n", fname);
		return ERR_DL_PAGE;
	}
	if ((webpage = htm_alloc(fname, 0, NULL)) == NULL) {
		return ERR_MEMORY;
        }
	rc = hentaiera_process_page(webpage, fname);
	if (!cflags_check(CFLAGS_KEEP_PAGE)) {
		unlink(fname);
	}

	if (strcmp(_session_dir, ".")) {
                chdir("..");
        }
	free(webpage);
	return rc;
}

static int hentaiera_session_url(char *url)
{
	char	*webpage, *p, fname[1024];
	int	rc, skip, taken, curr, last;

	skip = taken = curr = last = rc = 0;
	while ((p = strstr(url, "/view/")) != NULL) {
		strx_strncpy(fname, p + 6, sizeof(fname));
		url_trim_tail_ctlsp(fname, "/");
		strx_strrpch(fname, 0, '/', '-');

		if (sys_download_wget_page(url, fname) < 0) {
			slog("%s : failed to retrieve the web page.\n", fname);
			return ERR_DL_PAGE;
		}
		if ((webpage = htm_alloc(fname, 0, NULL)) == NULL) {
			return ERR_MEMORY;
        	}
		if (!cflags_check(CFLAGS_KEEP_PAGE)) {
			unlink(fname);
		}

		if (hentaiera_session_attr(webpage, &curr, &last) != ERR_NONE) {
			free(webpage);
			return ERR_PARSE;
		}
		rc = hentaiera_session_page(webpage, fname);
		switch(rc) {
		case ERR_NONE:		/* successfully get the image and next url */
			strcpy(url, webpage);
			taken++;
			break;
		case ERR_DL_IMAGE:	/* failed to get the image but get the next url */
			strcpy(url, webpage);
			skip++;
			break;
		case ERR_URL_NEXT:	/* successfully get the image but no next url */
			url[0] = 0;
			taken++;
			break;
		case ERR_URL_NONE:
		default:	/* failed to get the image and next url */
			url[0] = 0;
			skip++;
			break;
		}
		free(webpage);
		printf("Waiting %d seconds.\n", sys_delay());
	}
	switch (rc) {
	case ERR_PARSE:
		slog("%s : parse page failed %s\n", _session_dir, fname);
		if (last > 0) {
			skip += last - curr + 1;
                }
                break;
	case ERR_URL_NEXT: 
		slog("%s : %d downloaded; %d missed\n", _session_dir, taken, skip);
		if (last > 0) {
			skip += last - curr;
                }
                break;
	case ERR_URL_NONE:
	default:	/* failed to get the image and next url */
		if (last > 0) {
			skip += last - curr;
                }
                break;
	}
	_img_loaded += taken;
	_img_skipped += skip;
	return 0;	/* successfully run to the end page */
}


/* HOME:  <input type="hidden" name="gallery_id" id="gallery_id" value="912729" />
 * IMAGE: <a href="/view/912729/1/">
 * </svg>25 Pages</button>
 * <title> VACBED KITSUNE | LATEX Orgasm Denial | Vibrators | FETISH ABYSS - HentaiEra </title>
 * <input type="hidden" name="gallery_title" id="gallery_title" 
 * value="VACBED KITSUNE | LATEX Orgasm Denial | Vibrators | FETISH ABYSS [English]" />
 * Tag begin: <span class='tags_text'> Tags </span>
 * Tags: <span class='item_name'> big ass </span>
 * Tag end: <span class='tags_text'> Artists </span>
 * Artists: <span class='tags_text'> Artists </span><span class='item_name'> fetish studio </span>
 */
static int hentaiera_front_page(char *webpage, char *fpage)
{
	FILE	*fp = stdout;
	char	*bp, *ep, doc[1024];

	if (strstr(webpage, "<a href=\"/view/") == NULL) {
		return ERR_PARSE;	/* not found th session URL */
	}

	/* create the profile */
	strx_strncpy(doc, fpage, sizeof(doc));
	strcat(doc, ".txt");
	if ((fp = fopen(doc, "w")) == NULL) {
		fp = stdout;
	} else {
		/* Pluma largely requires 4-byte in BOM * likely a bug in Pluma *
	 	* so added an extra LR which seemingly harmless */
		fputs("\xEF\xBB\xBF\x0A", fp);
	}

	if (htm_doc_pick(webpage, NULL, "<title", "</title>", doc, sizeof(doc))) {
		fprintf(fp, "Title: %s\n", doc);
	}
	if ((bp = strx_strstr(webpage, NULL, "<input", ">", "id=\"gallery_title\"", NULL)) != NULL) {
		if (htm_tag_pick(bp, "value=\"", "\"", doc, sizeof(doc))) {
			fprintf(fp, "Title: %s\n", doc);
		}
	}
	if (strx_strstr(webpage, &ep, "<span", "</span>", "Tags", NULL) != NULL) {
		fprintf(fp, "Tags: ");
		while ((ep = strstr(ep, "<span")) != NULL) {
			if (htm_tag_pick(ep, "class='", "'", doc, sizeof(doc)) == NULL) {
				break;
			}
			if (strcmp(doc, "item_name")) {
				//fprintf(fp, "Break [%s]\n", doc);
				break;
			}
			if (htm_doc_pick(ep, NULL, "<span", "</span>", doc, sizeof(doc))) {
				fprintf(fp, "[%s] ", doc);
			}
			ep = strstr(ep, "</span>") + 7;
		}
		fprintf(fp, "\n");
	}
	if (strx_strstr(webpage, &ep, "<span", "</span>", "Artists", NULL) != NULL) {
		if (htm_doc_pick(ep, NULL, "<span", "</span>", doc, sizeof(doc))) {
                	fprintf(fp, "Artist: %s\n", doc);
		}
	}
	if ((bp = strx_strstr(webpage, NULL, "</svg>", "</button>", "Pages", NULL)) != NULL) {
		if (htm_doc_pick(bp, NULL, "</svg", "</button>", doc, sizeof(doc))) {
			fprintf(fp, "Total: %s\n", doc);
		}
	}
	if ((bp = strx_strstr(webpage, NULL, "<input", ">", "id=\"gallery_id\"", NULL)) != NULL) {
		if (htm_tag_pick(bp, "value=\"", "\"", doc, sizeof(doc))) {
			fprintf(fp, "HOME: https://hentaiera.com/gallery/%s\n", doc);
		}
	}

	bp = strstr(webpage, "<a href=\"/view/");
	htm_tag_pick(bp, "href=\"", "\"", doc, sizeof(doc));
	fprintf(fp, "URL:  https://hentaiera.com%s\n", doc);
	sprintf(webpage, "https://hentaiera.com%s", doc);

	if (fp != stdout) {
		fclose(fp);
	}
	return ERR_NONE;
}


/* <a href="/gallery/912729/" class="return_btn btn btn_colored">Back to gallery</a>
 * <input type="hidden" name="u_id" id="u_id" value="912729" />
 * <span class="total_pages"> 25 </span>
 * <button onclick="jumpPage(25);" class="btn_pages"> Page <span class="current"> 1 </span>
 *   <span class="separator"> of </span> <span class="total_pages"> 25 </span> </button>
 * <a class="next_img"><img id="gimg" class="lazy preloader image_1" 
 *   src="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='5 0 70 70'%3E%3C/svg%3E" 
 *   data-src="https://m6.hentaiera.com/021/2izsrj4e9a/1.jpg" 
 *   alt="VACBED KITSUNE | LATEX Orgasm Denial | Vibrators | FETISH ABYSS page 1 full" 
 *   style="max-width:1280px;max-height:2020px;margin:0 auto;display:block;" /></a>
 */
/* 20220912: "u_id" looks not reliable. Using "/gallery/912729/" instead */
static int hentaiera_session_page(char *webpage, char *fname)
{
	char	*bp, u_id[128], doc[1024];
	int	curr, last;

	/* find the front page for reference */
	if ((bp = strx_strstr(webpage, NULL, "<a href=", "</a>", "return_btn", "Back", NULL)) == NULL) {
		return ERR_PARSE;
	} else if (htm_tag_pick(bp, "href=\"", "\"", doc, sizeof(doc)) == NULL) {
		return ERR_PARSE;
	}
	strx_strncpy(u_id, url_get_tail(doc, '/'), sizeof(u_id));
	if (cflags_check(CFLAGS_DUMP)) {
		printf("HOME:    https://hentaiera.com%s\n", doc);
		printf("Front:   https://hentaiera.com/gallery/%s\n", u_id);
	}

	/* find the gallery id */
	/*if ((bp = strx_strstr(webpage, NULL, "<input", ">", "u_id", NULL)) == NULL) {
		return ERR_PARSE;
	} else if (htm_tag_pick(bp, "value=\"", "\"", u_id, sizeof(u_id)) == NULL) {
		return ERR_PARSE;
	} else if (cflags_check(CFLAGS_DUMP)) {
		printf("Front:   https://hentaiera.com/gallery/%s\n", u_id);
	}*/

	if (hentaiera_session_attr(webpage, &curr, &last) != ERR_NONE) {
		return ERR_PARSE;
	}
	if (cflags_check(CFLAGS_DUMP)) {
		printf("First:   https://hentaiera.com/view/%s/1\n", u_id);
		printf("Current: https://hentaiera.com/view/%s/%d\n", u_id, curr);
		printf("Next:    https://hentaiera.com/view/%s/%d\n", u_id, 
				curr < last ? curr + 1 : last);
		printf("Last:    https://hentaiera.com/view/%s/%d\n", u_id, last);
	}

	if ((bp = strstr(webpage, "<img id=\"gimg\"")) == NULL) {
		goto session_no_image;
	}
	if (htm_tag_pick(bp, "data-src=\"", "\"", doc, sizeof(doc)) == NULL) {
		goto session_no_image;
	}
	if (cflags_check(CFLAGS_DUMP)) {
		printf("URL:     %s\n", doc);
	}

	if (cflags_check(CFLAGS_MEDIA)) {
		if (sys_download_wget_image(doc, hentaiera_image_name(doc, curr, last)) != 0) {
			slog("%s : failed to get %s\n", fname, url_get_tail(doc, '/'));
			goto session_no_image;
		}
	}
	if (!cflags_check(CFLAGS_CONTINUOUS) || (curr >= last)) {
		/* successfully get the image but no next url.
		 * ! CFLAGS_MEDIA is considered as successfully getting the image */
		return ERR_URL_NEXT;	
	}
	/* return the next url in the webpage.
	 * warning: the input webpage will be destroied */
	sprintf(webpage, "https://hentaiera.com/view/%s/%d", u_id, curr + 1);
	return ERR_NONE;

session_no_image:
	if (!cflags_check(CFLAGS_CONTINUOUS) || (curr >= last)) {
		return ERR_URL_NONE;	/* no image, no next url */
	}
	/* return the next url in the webpage.
	 * warning: the input webpage will be destroied */
	sprintf(webpage, "https://hentaiera.com/view/%s/%d", u_id, curr + 1);
	return ERR_DL_IMAGE;	/* no image, has got the next url */
}

static int hentaiera_session_attr(char *webpage, int *curr, int *last)
{
	char	*bp, doc[1024];

	if ((bp = strx_strstr(webpage, NULL, "<button", "</button>", "btn_pages", "Page", NULL)) == NULL) {
		return ERR_PARSE;
	}
	
	if (htm_doc_pick(bp, NULL, "current\"", "</span>", doc, sizeof(doc)) == NULL) {
		return ERR_PARSE;
	}
	if (curr) {
		*curr = (int)strtol(doc, NULL, 0);
	}
		
	if (htm_doc_pick(bp, NULL, "total_pages\"", "</span>", doc, sizeof(doc)) == NULL) {
		return ERR_PARSE;
	}
	if (last) {
		*last = (int)strtol(doc, NULL, 0);
	}
	return ERR_NONE;
}

static char *hentaiera_image_name(char *imgurl, int curr, int last)
{
	static	char	buffer[1024];

	buffer[0] = 0;
	if (cflags_check(CFLAGS_SORT)) {
		strcat(buffer, url_image_sn(curr, last));
		strcat(buffer, "_");
	}
	strcat(buffer, url_get_tail(imgurl, '/'));
	return buffer;
}




