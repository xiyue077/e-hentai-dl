/* testing https://www.xvideos.com/video33916411/freezing_vibration_ova */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "urltool.h"

#define URL_MAX		32

static int xvideos_sort_m3u8(char *url);

/*
static int print60(int ref, char *s)
{
	char	tmp[256];

	strx_strncpy(tmp, s, 60);
	printf("%d: %s\n", ref, tmp);
	return 0;
}
*/

/* https://www.xvideos.com/video33916411/freezing_vibration_ova
 * <title>Freezing Vibration OVA - XVIDEOS.COM</title>
 * "video_title":"Freezing Vibration OVA"
 * html5player.setVideoTitle('Freezing Vibration OVA');
"contentUrl": "https://video-hw.xvideos-cdn.com/videos/mp4/3/f/6/xvideos.com_
  3f6273a9505db8598b7751ad53b3e206.mp4?e=1662387291&ri=1024&rs=85&h=61af43d6459cc28bc5842c03eea9860a",
<a href="https://video-hw.xvideos-cdn.com/videos/mp4/3/f/6/xvideos.com_
  3f6273a9505db8598b7751ad53b3e206.mp4?e=1662387291&ri=1024&rs=85&h=61af43d6459cc28bc5842c03eea9860a">
<a href="https://video-hw.xvideos-cdn.com/videos/3gp/3/f/6/xvideos.com_
  3f6273a9505db8598b7751ad53b3e206.mp4?e=1662387291&ri=1024&rs=85&h=b54d1b53c573ca65aa100722b69bf3ce">
<a href="https://video-hw.xvideos-cdn.com/videos/mp4/3/f/6/xvideos.com_
  3f6273a9505db8598b7751ad53b3e206.mp4?e=1662387291&ri=1024&rs=85&h=61af43d6459cc28bc5842c03eea9860a">
html5player.setVideoUrlLow('https://video-hw.xvideos-cdn.com/videos/3gp/3/f/6/xvideos.com_
  3f6273a9505db8598b7751ad53b3e206.mp4?e=1662387291&ri=1024&rs=85&h=b54d1b53c573ca65aa100722b69bf3ce');
html5player.setVideoUrlHigh('https://video-hw.xvideos-cdn.com/videos/mp4/3/f/6/xvideos.com_
  3f6273a9505db8598b7751ad53b3e206.mp4?e=1662387291&ri=1024&rs=85&h=61af43d6459cc28bc5842c03eea9860a');
 */
static int xvideos_video_page(char *webpage, char *fpage)
{
	char	*next, *urlidx[URL_MAX];
	char	vidlink[1024], title[1024];
	int	i, rc = 0;

	/* find the title to be the file name */
	if (fpage) {
		strx_strncpy(title, fpage, sizeof(title)-8);
	} else if ((next = strstr(webpage, "\"video_title\"")) != NULL) {
		htm_tag_pick(next, "\"video_title\":\"", "\"", title, sizeof(title)-8);
	} else if ((next = strstr(webpage, "html5player.setVideoTitle")) != NULL) {
		htm_tag_pick(next, "setVideoTitle('", "'", title, sizeof(title)-8);
	} else if (htm_doc_pick(webpage, NULL, "<title", "</title>", title, sizeof(title)-24)) {
		if ((next = strstr(title, " - XVIDEOS.COM")) != NULL) {
			*next = 0;
		}
		strx_strrpch(title, 0, ' ', '_');
	} else {
		title[0] = 0;
	}

	/* List all medias available 
	 * searching player control
	 * html5player.setVideoUrlLow('https://video-hw.xvideos ...
	 * html5player.setVideoUrlHigh('https://video-hw.xvideos-c ...
	 * html5player.setVideoHLS('http://cdn77-vid.xvideos-cdn.com ...
	 * Pointing to:
	 * UrlLow('https://video-hw.xvideos
	 */
	for (i = 0, next = webpage; i < URL_MAX; i++) {
		if ((urlidx[i] = strstr(next, "html5player.setVideo")) == NULL) {
			break;
		}
		urlidx[i] += strlen("html5player.setVideo");
		next = strchr(urlidx[i], ')') + 1;

		if (strx_strncmp(urlidx[i], "Url") && strx_strncmp(urlidx[i], "HLS")) {
			urlidx[i--] = NULL;
			continue;
                }
		//print60(i, urlidx[i]);
	}

	/* Find contentUrl:
	 * "contentUrl": "https://video-hw.xvideos-cdn.com/videos/mp4/3/f/6/xvideos.com_306.mp4?e"
	 * Pointing to:
	 * https://video-hw.xvideos-c ...
	 */
	for (next = webpage; i < URL_MAX; i++) {
		if ((urlidx[i] = strstr(next, "\"contentUrl\":")) == NULL) {
			break;
		}
		next = urlidx[i] + 13;
		urlidx[i] = strchr(next, '\"') + 1;
		//print60(i+10, urlidx[i]);
	}

	/* searching video resources 
	 * <a href="https://video-hw.xvideos-cdn.com/videos/mp4/3/f/6/xvideo ..."
	 * Pointing to:
	 * https://video-hw.xvideos-c ...
	 */
	for (next = webpage; i < URL_MAX; i++) {
		if ((urlidx[i] = strstr(next, "<a href=\"https://")) == NULL) {
			break;
		}
		urlidx[i] = htm_tag_pick(urlidx[i], "<a href=\"", "\"", vidlink, sizeof(vidlink));
		next = strchr(urlidx[i], '>');
		if (strstr(vidlink, ".mp4") == NULL) {
			urlidx[i--] = NULL;
			continue;
		}
		//print60(i+20, urlidx[i]);
	}

	if (cflags_check(CFLAGS_DUMP)) {
		for (i = 0; urlidx[i]; i++) {
			if (!strx_strncmp(urlidx[i], "http")) {
				htm_common_pick(urlidx[i], NULL, "\"", vidlink, sizeof(vidlink));
			} else {
				htm_common_pick(urlidx[i], "('", "')", vidlink, sizeof(vidlink));
			}
			printf("%d: %s\n", i, vidlink);
		}
	}

	if (urlidx[0] == NULL) {
		slog("%s: video link not found.\n", fpage);
		return ERR_URL_NONE;
	}

	/* pick high resolution first; otherwise choose the last one */
	for (i = 0; urlidx[i]; i++) {
		if (!strx_strncmp(urlidx[i], "HLS(")) {
			goto foundit;
		}
	}
	for (i = 0; urlidx[i]; i++) {
		if (!strx_strncmp(urlidx[i], "UrlHigh(")) {
			goto foundit;
		}
	}

foundit:
	if (urlidx[i] == NULL) {		/* no high resolution profile */
		i--;
		htm_common_pick(urlidx[i], NULL, "\"", vidlink, sizeof(vidlink));
	} else {
		htm_common_pick(urlidx[i], "('", "')", vidlink, sizeof(vidlink));
	}

	//cflags_clear(CFLAGS_MEDIA);

	if (title[0] == 0) {
		htm_common_pick(url_get_tail(vidlink, '/'), NULL, "?", title, sizeof(title));
	} else {
		strcat(title, ".mp4");
	}

	printf("Downloading %s ...\n", vidlink);
	if (cflags_check(CFLAGS_MEDIA)) {
		if (!strx_strncmp(urlidx[i], "HLS(")) {
			xvideos_sort_m3u8(vidlink);
			printf("Refined %s ...\n", vidlink);
			rc = sys_download_m3u8(vidlink, title);
		} else {
			rc = sys_download_wget_image(vidlink, title);
		}
		if (rc == 0) {
			slog("%s: saved '%s'\n", fpage, title);
		} else {
			slog("%s: download video failed.\n", fpage);
		}
	}
	return rc;
}

int xvideos_process_page(char *webpage, char *fpage)
{
	return xvideos_video_page(webpage, fpage);
}

int xvideos_process_url(char *url)
{
	char	*fname, *webpage;

	fname = url_get_tail(url, '/');
	if (sys_download_wget_page(url, fname) < 0) {
		return -1;
	}
	if ((webpage = htm_alloc(fname, 0, NULL)) == NULL) {
		return -2;
        }
	xvideos_process_page(webpage, fname);
	free(webpage);

	if (!cflags_check(CFLAGS_KEEP_PAGE)) {
		unlink(fname);
	}
	return 0;
}

static int xvideos_sort_m3u8(char *url)
{
	FILE	*fp;
	char	*fname, buf[1024], best[256];
	int	rc, res = 0;

	fname = url_get_tail(url, '/');
	if (sys_download_wget(url, fname) != 0) {
		return -1;
	}
	
	if ((fp = fopen(fname, "r")) == NULL) {
		return -1;
	}
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (buf[0] == '#') {
			continue;
		}
		url_trim_tail_ctlsp(buf, NULL);

		/* hls-360p-27087.m3u8 */
		rc = (int)strtol(buf + 4, NULL, 0);
		if (rc > res) {
			res = rc;
			strx_strncpy(best, buf, sizeof(best));
		}
	}
	fclose(fp);

	strcpy(fname, best);
	return 0;
}


