
#include "tinylib/rtsp/sdp.h"
#include "tinylib/util/log.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static inline void sdp_session_online(sdp_session_t* session, char *line, unsigned len)
{
    char key;
    char *part;
    char *pos;
    sdp_attrib_t *attrib;

    assert(NULL != session && NULL != line && 0 < len);

    while (*line == ' ' && *line == '\t') line++;
    pos = strchr(line, '=');
    if (NULL == pos)
    {
        /* ��������û��=������һ���Ϸ���session�У����������� */
        return;
    }

    pos++;
    key = *line;
    switch(key)
    {
        case 'v':
        {
            session->version = pos;
            break;
        }
        case 'o':
        {
            session->origin_user = pos;

            part = pos;
            pos = strchr(part, ' ');
            if (NULL == pos)
            {
                pos = strchr(part, '\t');
            }
            assert(NULL != pos);
            *pos = '\0';
            pos++;
            session->origin_session_id = pos;
            
            part = pos;
            pos = strchr(part, ' ');
            if (NULL == pos)
            {
                pos = strchr(part, '\t');
            }
            assert(NULL != pos);
            *pos = '\0';
            pos++;
            session->origin_session_ver = pos;
            
            part = pos;
            pos = strchr(part, ' ');
            if (NULL == pos)
            {
                pos = strchr(part, '\t');
            }
            assert(NULL != pos);
            *pos = '\0';
            pos++;
            session->origin_net_type = pos;

            part = pos;
            pos = strchr(part, ' ');
            if (NULL == pos)
            {
                pos = strchr(part, '\t');
            }
            assert(NULL != pos);
            *pos = '\0';
            pos++;
            session->origin_addr_type = pos;

            part = pos;
            pos = strchr(part, ' ');
            if (NULL == pos)
            {
                pos = strchr(part, '\t');
            }
            assert(NULL != pos);
            *pos = '\0';
            pos++;
            session->origin_addr = pos;
            
            break;
        }
        case 's':
        {
            session->name = pos;
            break;
        }
        case 'e':
        {
            session->email = pos;
            break;
        }
        case 'b':
        {
            session->bandwidth = pos;
            break;
        }
        case 't':
        {
            session->time = pos;
            break;
        }
        case 'a':
        {
            part = pos;
            pos = strchr(part, ':');
            if (NULL == pos)
            {
                /* �������У�û��ð�Ų���һ���Ϸ��������У����� */
                break;
            }

            *pos = '\0';
            pos++;

            if (strncasecmp(part, "control", 7) == 0)
            {
                session->control = pos;
                break;
            }

            attrib = (sdp_attrib_t*)malloc(sizeof(sdp_attrib_t));
            memset(attrib, 0, sizeof(*attrib));
            attrib->key = part;
            attrib->value = pos;
            attrib->next = NULL;

            if (NULL == session->attrib)
            {
                session->attrib = attrib;
                session->attrib_end = attrib;
            }
            else
            {
                ((sdp_attrib_t*)session->attrib_end)->next = attrib;
                session->attrib_end = attrib;
            }

            break;
        }
        default:
        {
            break;
        }
    }

    return;
}

static inline void sdp_media_online(sdp_media_t* media, char *line, unsigned len)
{
    char key;
    char *part;
    char *pos;
    sdp_attrib_t *attrib;

    assert(NULL != media && NULL != line && 0 < line);

    while (*line == ' ' && *line == '\t') line++;
    pos = strchr(line, '=');
    if (NULL == pos)
    {
        /* ���������в�����=������һ���Ϸ��������� */
        return;
    }

    pos++;
    key = *line;
    switch(key)
    {
        case 'b':
        {
            media->bandwidth = pos;
            break;
        }
        case 'a':
        {
            part = pos;
            pos = strchr(part, ':');
            if (NULL == pos)
            {
                /* �������������в�����ð�ţ�����һ���Ϸ��������У����� */
                break;
            }

            *pos = '\0';
            pos++;

            if (strncasecmp(part, "control", 7) == 0)
            {
                media->control = pos;
                break;
            }

            attrib = (sdp_attrib_t*)malloc(sizeof(sdp_attrib_t));
            memset(attrib, 0, sizeof(*attrib));
            attrib->key = part;
            attrib->value = pos;

            if (NULL == media->attrib)
            {
                media->attrib = attrib;
                media->attrib_end = attrib;
            }
            else
            {
                ((sdp_attrib_t*)media->attrib_end)->next = attrib;
                media->attrib_end = attrib;
            }

            break;
        }
        default:
        {
            break;
        }
    }

    return;
}

sdp_t* sdp_parse(const char *data, unsigned len)
{
    char *line;
    char *pos;
    sdp_t *sdp;
    char *crlf;
    sdp_media_t *media;
    char key;
    char *rawdata;
    unsigned media_count;

    if (NULL == data || 0 == len)
    {
        log_error("sdp_parse: bad data(%p) or bad len(%u)", data, len);
        return NULL;
    }

    sdp = (sdp_t*)malloc(sizeof(*sdp) + len + 1);
    memset(sdp, 0, (sizeof(*sdp) + len + 1));
    rawdata = (char*)sdp + sizeof(*sdp);
    memcpy(rawdata, data, len);    

    /* ȷ�ϵ�һ������session����media */
    line = rawdata;    
    while(*line == ' ' || *line == '\t') line++;
    pos = strchr(line, '=');    
    if (NULL == pos)
    {
        free(sdp);
        log_error("sdp_parse: no = in sdp data, not a valid sdp");
        return NULL;
    }

    key = *line;

    media_count = 0;
    sdp->media[0] = NULL;
    sdp->media[1] = NULL;
    sdp->media[2] = NULL;

    /* ��һ��key=value����m=������sdp��Ϣ����session�ο�ʼ�� */
    if (key != 'm')
    {
        sdp->session = (sdp_session_t*)malloc(sizeof(sdp_session_t));
        memset(sdp->session, 0, sizeof(sdp_session_t));

        do
        {
            crlf = strstr(line, "\r\n");
            if (NULL == crlf)
            {
                /* �������һ�е����� */
                sdp_session_online(sdp->session, line, (len - (line - rawdata)));

                return sdp;
            }

            *crlf = '\0';
            sdp_session_online(sdp->session, line, (crlf - line + 2));

            line = crlf+2;
            while(*line == ' ' || *line == '\t') line++;
            key = *line;
        } while(key != 'm');
    }

    /* ���˿�ʼ��media�Ķ� */
    do
    {
        while (*line == ' ' || *line == '\t') line++;

        crlf = strstr(line, "\r\n");
        key = *line;

        if (NULL != crlf)
        {
            *crlf = '\0';
        }

        if ('m' == key)
        {
            pos = strchr(line, '=');
            if (NULL == pos)
            {
                /* ʣ�²���������û��=�����ǺϷ���sdp���ݣ�����֮���������� */
                return sdp;
            }

            /* ����m= */
            pos++;
            line = pos;
            pos = strchr(line, ' ');
            if (NULL == pos)
            {
                pos = strchr(line, '\t');
            }

            if (NULL == pos)
            {
                /* media��ʼ�и�ʽ�Ƿ���ʣ�µ�����Ϊ�Ƿ�������֮���������� */
                log_error("sdp_parse(): bad media start line");
                return sdp;
            }
            
            *pos = '\0';
            pos++;
            while(*pos == ' ' || *pos == '\t') pos++;

            /* ����������media�� */
            if (media_count >= 3)
            {
                break;
            }

            media = (sdp_media_t*)malloc(sizeof(sdp_media_t));
            sdp->media[media_count] = media;
            media_count++;

            memset(media, 0, sizeof(*media));
            media->type = line;
            media->param = pos;
        }    

        if (NULL == crlf)
        {
            /* û�и�����У���Ϊ�����һ�� */
            sdp_media_online(media, line, (len - (line - rawdata)));
            break;
        }
        else
        {
            *crlf = '\0';
            sdp_media_online(media, line, (crlf-line+2));
            line = crlf+2;
        }
    } while((line - rawdata) < (int)len);

    return sdp;
}

void sdp_destroy(sdp_t* sdp)
{
    int i;
    sdp_media_t *media;
    const sdp_attrib_t *attrib;
    const sdp_attrib_t *a_iter;

    if (NULL == sdp)
    {
        return;
    }

    if (NULL != sdp->session)
    {
        attrib = sdp->session->attrib;
        while (NULL != attrib)
        {
            a_iter = attrib->next;
            free((void*)attrib);
            attrib = a_iter;
        }
        
        free(sdp->session);
    }

    for (i = 0; i < 3; ++i)
    {
        media = sdp->media[i];
        if (NULL == media)
        {
            continue;
        }

        attrib = media->attrib;
        while (NULL != attrib)
        {
            a_iter = attrib->next;
            free((void*)attrib);
            attrib = a_iter;
        }

        free(media);
    }
    
    free(sdp);

    return;
}

