#ifndef _URLTOOL_H_
#define _URLTOOL_H_

#define ERR_NONE		0
#define ERR_DL_PAGE		-1
#define ERR_DL_IMAGE		-2
#define ERR_PARSE		-3
#define ERR_URL_LAST		-4
#define ERR_URL_NEXT		-5
#define ERR_TRAP		-6
#define ERR_URL_NONE		-7
#define ERR_MEMORY		-8

#define CFLAGS_DUMP		1
#define CFLAGS_DUMP_ALL		2
#define CFLAGS_DUMP_MEDIA	4
#define CFLAGS_CONTINUOUS	0x100
#define CFLAGS_SORT		0x200
#define CFLAGS_MEDIA		0x400
#define CFLAGS_KEEP_PAGE	0x800

#define URL_CMD_FIRST		0
#define URL_CMD_LAST		1
#define URL_CMD_NEXT		2
#define URL_CMD_PREV		3
#define URL_CMD_IMAGE		4
#define URL_CMD_BACK		5
#define URL_CMD_ORIGIN		6
#define URL_CMD_ALL		9

#define DOWNLOAD_INTERVAL	5

#ifdef __cplusplus
extern "C"
{
#endif

int urltool(void);
int url_trim_tail(char *url, char *cset);
int url_trim_tail_ctlsp(char *url, char *cset);
char *url_skip_head_ctlsp(char *url, char *cset);
char *url_get_tail(char *url, int sep);
char *url_get_path(char *url, int pn, char *buf, int blen);
char *url_reform(char *url, char *buffer, int blen);
int url_get_index(char *s, char *key, int klen);
int url_is_remote(char *s);
int url_is_image(char *url);
char *url_image_sn(int curr, int last);

int strx_blankline(char *s);
char *strx_alloc(char *s, unsigned extra);
char *strx_strncpy(char *dest, const char *src, size_t n);
int strx_strncmp(char *dest, const char *src);
int strx_memcpy(char *dest, int dlen, char *from, char *to);
void strx_puts(char *s, int n);
void strx_putx(char *s, int n);
int strx_strrpch(char *s, int num, int old, int newx);
char *strx_strstr(char *s, char **endp, char *first, char *last, ...);

void *htm_alloc(char *fname, int exhead, int *flen);
char *htm_doc_pick(char *s, char **sp, char *from, char *to, char *buf, int blen);
char *htm_tag_pick(char *s, char *from, char *to, char *buf, int blen);
char *htm_common_pick(char *s, char *from, char *to, char *buf, int blen);
int htm_break(char *fname);

int sys_download_ytdl(char *url);
int sys_download_m3u8(char *url, char *fname);
int sys_download_wget(char *url, char *fname);
int sys_download_wget_image(char *url, char *fname);
int sys_download_wget_page(char *url, char *fname);
int sys_download_proxy_open(char *proxy);
void sys_download_proxy_close(void);
int sys_download_cookies_open(char *cookie);
void sys_download_cookies_close(void);
int sys_delay(void);

int cflags_argvs(int c);
int cflags_set(int flag);
int cflags_clear(int flag);
int cflags_check(int flag);

int slog_open(char *logname);
int slog_close(void);
int slog(char *fmt, ...);

#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif
