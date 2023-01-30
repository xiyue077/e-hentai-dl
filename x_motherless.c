/* <a href="https://motherless.com/F3C9950" title="22-037" class="caption title pop plain">22-037</a>
 *
 * gcc -Wall -DMOTHERLESS_MAIN -o motherless motherless.c urltool.c
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "urltool.h"

#define URL_MAX		32


/* https://motherless.com/u/Corpse_Lover50 
 * <a href="https://motherless.com/F3C9950" title="22-037" class="caption title pop plain">22-037</a>
 */
static int motherless_user_page(char *webpage, char *fpage)
{
	char	title[128], url[512];
	char	*fname, *next;

	for (next = webpage; (next = strstr(next, "<a href=")) != NULL; next = strchr(next, '>')) {
		if (htm_tag_pick(next, "class=\"", "\"", url, sizeof(url)) == NULL) {
			continue;
		}
		if (strcmp(url, "caption title pop plain")) {
			continue;
		}
		if (htm_tag_pick(next, "title=\"", "\"", title, sizeof(title)) == NULL) {
			continue;
		}
		htm_tag_pick(next, "href=\"", "\"", url, sizeof(url));
		fname = url_get_tail(url, '/');
		printf("wget -O %s_%s.mp4 https://cdn5-videos.motherlessmedia.com/videos/%s.mp4\n",
				title, fname, fname);
	}
	return 0;
}

/* https://motherless.com/F3C9950
 * <title>22-037 | MOTHERLESS.COM ™</title>
 * __fileurl = 'https://cdn5-videos.motherlessmedia.com/videos/F3C9950.mp4';
 * <source src="https://cdn5-videos.motherlessmedia.com/videos/F3C9950.mp4" type="video/mp4" res="480p" label="SD">
 * <source src="https://cdn5-videos.motherlessmedia.com/videos/F3C9950-720p.mp4" type="video/mp4" res="720p" label="HD">
 */
static int motherless_video_page(char *webpage, char *fpage)
{
	char	*next, *urlidx[URL_MAX];
	char	vidlink[256], title[256];
	int	i, rc = 0;

	/* find the title to be the file name */
	if (htm_doc_pick(webpage, NULL, "<title", "</title>", title, sizeof(title))) {
		if ((next = strchr(title, '|')) != NULL) {
			for (next--; *next <= ' '; *next-- = 0);
		}
	}

	/* find the list of the possible media.
	 * It looks the last one is the high solution one */
	i = 0;
	if ((urlidx[0] = strstr(webpage, "__fileurl")) != NULL) {
		i = 1;
	}
	for (next = webpage; i < URL_MAX; i++) {
		if ((urlidx[i] = strstr(next, "<source src=")) == NULL) {
			break;
		}
		next = strchr(urlidx[i], '>');
	}
	
	/* easy pick: the last one */
	for (i = 0; urlidx[i]; i++) {
		if (!strx_strncmp(urlidx[i], "__fileurl")) {
			htm_common_pick(urlidx[i], "'", "'", vidlink, sizeof(vidlink));
		} else {
			htm_tag_pick(urlidx[i], "src=\"", "\"", vidlink, sizeof(vidlink));
		}
		if (cflags_check(CFLAGS_DUMP)) {
			printf("%d: %s\n", i, vidlink);
		}
	}

	if (i == 0) {
		slog("%s: video link not found.\n", fpage);
		return ERR_URL_NONE;
	}

	strcat(title, "_");
	strcat(title, url_get_tail(vidlink, '/'));
	printf("Downloading %s ...\n", vidlink);
	if (cflags_check(CFLAGS_MEDIA)) {
		rc = sys_download_wget_image(vidlink, title);
		if (rc == 0) {
			slog("%s: saved '%s'\n", fpage, title);
		} else {
			slog("%s: download video failed.\n", fpage);
		}
	}
	return rc;
}

int motherless_process_page(char *webpage, char *fpage)
{
	/* https://motherless.com/u/Corpse_Lover50 :
	 * <meta name="referrer" content="origin-when-cross-origin">
	 * <link rel="canonical" href="https://motherless.com/u/Corpse_Lover50"/>
	 *
	 * https://motherless.com/F3C9950 :
	 * <meta name="referrer" content="origin-when-cross-origin">
	 * <meta property="og:image" content="https://cdn5-thumbs.motherlessmedia.com/
	 *     thumbs/F3C9950-small-7.jpg">
	 * <link rel="image_src" type="image/image/jpg" href="https://cdn5-thumbs.
	 *    motherlessmedia.com/thumbs/F3C9950-small-7.*/
	if (strstr(webpage, "<meta property=")) {
		motherless_video_page(webpage, fpage);
	} else {
		motherless_user_page(webpage, fpage);
	}
	return 0;
}

int motherless_process_url(char *url)
{
	char	*fname, *webpage;

	fname = url_get_tail(url, '/');
	if (sys_download_wget_page(url, fname) < 0) {
		return -1;
	}
	if ((webpage = htm_alloc(fname, 0, NULL)) == NULL) {
		return -2;
        }
	motherless_process_page(webpage, fname);
	free(webpage);

	if (!cflags_check(CFLAGS_KEEP_PAGE)) {
		unlink(fname);
	}
	return 0;
}

#ifdef	MOTHERLESS_MAIN
int main(int argc, char **argv)
{
	char	*argsave;

	if (argc < 2) {
		printf("Usage: %s [URL | html_file]\n", argv[0]);
		return 0;
	}

	if ((argsave = strx_alloc(argv[1], 0)) == NULL) {
		return -1;
	}
	
	cflags_set(CFLAGS_KEEP_PAGE);
	if (url_is_remote(argsave)) {
		motherless_process_url(argsave);
	} else {
		char *webpage = htm_alloc(argsave, 0, NULL);
		motherless_process_page(webpage, argsave);
		free(webpage);
	}
	free(argsave);
	return 0;
}
#endif

