/* */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "urltool.h"

#define URL_MAX		32

/* https://www.heavy-r.com/video/416067/Gaming_Characters/
 * <title>Gaming Characters</title>
 * <meta property="og:url" content="https://www.heavy-r.com/video/416067/Gaming_Characters/" />
 * <source type="video/mp4" src="https://cdn-b.heavy-r.com/vid/48/e7/71/48e7719cc546d9a.mp4">
 */
static int heavy_r_video_page(char *webpage, char *fpage)
{
	char	*next, *urlidx[URL_MAX];
	char	vidlink[256], title[256];
	int	i, rc = 0;

	/* find the title to be the file name */
	if (fpage) {
		strx_strncpy(title, fpage, sizeof(title)-24);
	} else if ((next = strstr(webpage, "<meta property=\"og:url\"")) != NULL) {
		htm_tag_pick(next, "content=\"", "\"", vidlink, sizeof(vidlink));
		strx_strncpy(title, url_get_tail(vidlink, '/'), sizeof(title)-24);
	} else if (htm_doc_pick(webpage, NULL, "<title", "</title>", title, sizeof(title)-24)) {
		strx_strrpch(title, 0, ' ', '_');
	} else {
		title[0] = 0;
	}

	/* find the list of the possible media.
         * It looks the last one is the high solution one */
	for (i = 0, next = webpage; i < URL_MAX; i++) {
		if ((urlidx[i] = strstr(next, "<source type=")) == NULL) {
			break;
		}
		next = strchr(urlidx[i], '>');
	}
	for (i = 0; urlidx[i]; i++) {
		htm_tag_pick(urlidx[i], "src=\"", "\"", vidlink, sizeof(vidlink));
		if (cflags_check(CFLAGS_DUMP)) {
			printf("%d: %s\n", i, vidlink);
		}
	}

	/* easy pick: the last one */
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

int heavy_r_process_page(char *webpage, char *fpage)
{
	return heavy_r_video_page(webpage, fpage);
}

int heavy_r_process_url(char *url)
{
	char	*fname, *webpage;

	fname = url_get_tail(url, '/');
	if (sys_download_wget_page(url, fname) < 0) {
		return -1;
	}
	if ((webpage = htm_alloc(fname, 0, NULL)) == NULL) {
		return -2;
        }
	heavy_r_process_page(webpage, fname);
	free(webpage);

	if (!cflags_check(CFLAGS_KEEP_PAGE)) {
		unlink(fname);
	}
	return 0;
}


