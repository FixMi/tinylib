
/** ʵ��һ��������URL�����Ĺ���*/

#ifndef UTIL_URL_H
#define UTIL_URL_H

/* rtsp://user:pass@10.0.0.1:554/demo.mp4/track1?key1=value1&key2=value2#hash */
typedef struct url
{
    const char *schema;               /* rtsp: */
    const char *user;                 /* user */
    const char *pass;                 /* pass */
    const char *host;                 /* 10.0.0.1 */
    unsigned short port;        	  /* 554 */
    const char *path;                 /* /demo.mp4/track1 */
	const char *query;				  /* key1=value1&key2=value2 */
	const char *hash;				  /* hash */
}url_t;

#ifdef __cplusplus
extern "C" {
#endif

/** ��������url�����������������url_t�Ľṹ���з���
 * ����ʧ��ʱ����NULL����ʾ����һ���Ϸ���url
 * �Է��ص�url_t��Ҫʹ��url_release()�����ͷ�
 */
url_t* url_parse(const char* url, unsigned len);

void url_release(url_t* url);

#ifdef __cplusplus
}
#endif

#endif /* !UTIL_URL_H */
