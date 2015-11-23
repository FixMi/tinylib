
#include "url.h"
#include "log.h"

#include <string.h>
#include <stdlib.h>

/* rtsp://user:pass@10.0.0.1:554/demo.mp4/track1 */

static inline void extrac_query_hash(url_t *u)
{
	char *q_pos;
	char *h_pos;
	
	q_pos = strchr(u->path, '?');
	h_pos = strchr(u->path, '#');

	if (NULL != q_pos)
	{
		*q_pos = '\0';
		q_pos++;
		u->query = q_pos;
	}

	if (NULL != h_pos)
	{
		*h_pos = '\0';
		h_pos++;
		u->hash = h_pos;
	}

	return;
}

url_t* url_parse(const char* url, unsigned len)
{
    char *u_raw;
    url_t *u;
    char *part;
    char *pos;
    char *pos1;

    if (NULL == url || 4 > len)
    {
        return NULL;
    }

	/* �˴������3����Ϊ\0�����ռ�֮�⣬
	 * ���������������У���Ҫ����ռ�
	 * 1, ����pathʱ����Ҫ��path���������ƶ�һ���ַ�λ��, ������Ҫһ��byte
	 * 2, ������pathʱ����Ҫ�ֶ�ָ��pathΪ"/", ������Ҫ2��byte
	 * 
	 * �ʴ˴�Ϊ+3
	 */	
    u = (url_t*)malloc(sizeof(url_t)+len+3);   
	
    memset(u, 0, sizeof(url_t)+len+3);
    u_raw = (char*)&u[1];
    strcpy(u_raw, url);

    u->schema = NULL;
    u->user = NULL;
    u->pass = NULL;
    u->host = NULL;
    u->port = 0;
    u->path = NULL;
	u->query = NULL;

    part = u_raw;
    pos = strstr(part, "://");
    if (NULL == pos)
    {
        log_error("url_parse: bad url(%s)", url);
        free(u);
        return NULL;
    }

    pos++;
    *pos = '\0';	/* ��ȡschema������� */
    u->schema = part;

    pos += 2;		/* ָ���ƶ��� user:pwd@host:port/path?query#hash ���� */
    part = pos;
	
	pos = strchr(part, '/');
	if (NULL == pos)
	{
		/* û��path���֣��ֶ���pathȷ��Ϊ"/" */
		pos = part + strlen(part) + 1;
		*pos = '/';
		u->path = pos;
	}
	else
	{
		/* ����path����/��ʼ��������ƶ�һ���ַ�������Ŀռ����ʱ���Ѿ��ṩ�� */
		memmove(pos+1, pos, strlen(pos));
		*pos = '\0';	/* path֮ǰ�Ĳ��ֽ�ȡ��� */

		pos++;
		u->path = pos;
		extrac_query_hash(u);
	}

	/* part -> user:pwd@host:port */

    pos = strrchr(part, '@');
    if (NULL != pos)
    {
		/* ���� user@pwd ���� */
        *pos = '\0';	/* ��ȡ user:pwd ������� */
        u->user = part;
        
        pos1 = strchr(part, ':');
        if (NULL != pos1)
        {
            *pos1 = '\0';	/* ��ȡuser������� */
            pos1++;
            if (pos1 == pos)
            {
				/* user:@host �龰��Ϊû������ */
                pos1 = NULL;
            }
        }
        u->pass = pos1;

        pos++;
        part = pos;
    }

    u->host = part;

    pos = strchr(u->host, ':');
    if (NULL != pos)
    {
        /* url���ж˿� */
        *pos = '\0';	/* ��ȡhost������� */

		/* atoi("123/path")���Ϊ123��atoi("/path")���Ϊ0�����Կ���ֱ�ӻ�ȡ�˿� */
        pos++;
		part = pos;
		u->port = atoi(part);
	}

    return u;
}

void url_release(url_t* url)
{
    free(url);

    return;
}

