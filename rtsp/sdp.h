
/** һ��SDP��һ��������������ֻ���򵥵��ı�������
 * �����ֵ���壬��ʹ���߸�����Ҫ��������һ������
 */

#ifndef RTSP_SDP_H
#define RTSP_SDP_H

typedef struct sdp_attrib
{
    const char* key;
    const char* value;
    const struct sdp_attrib *next;
}sdp_attrib_t;

/* sdp��Ϣ��session���ֵļ�¼ */
typedef struct sdp_session
{
    const char* version;
    const char* origin_user;
    const char* origin_session_id;
    const char* origin_session_ver;
    const char* origin_net_type;
    const char* origin_addr_type;
    const char* origin_addr;
    const char* name;
    const char* email;
    const char* bandwidth;
    const char* time;
    const char* control;
    
    const sdp_attrib_t *attrib;
	const sdp_attrib_t *attrib_end;
}sdp_session_t;

typedef struct sdp_media
{
    const char* type;                /* video/audio */
    const char* param;               /*m=vedio/audio֮������� */
    const char* bandwidth;
    const char* control; 
    const sdp_attrib_t* attrib;      /* ��control����֮��������attribute */
	const sdp_attrib_t *attrib_end;
}sdp_media_t;

typedef struct sdp
{
    sdp_session_t *session;
    sdp_media_t *media[3];      /* Ŀǰ������������ý�����ֱ�ʹ��tackID���б�ʾ */
}sdp_t;

#ifdef __cplusplus
extern "C" {
#endif

sdp_t* sdp_parse(const char *data, unsigned len);

void sdp_destroy(sdp_t* sdp);

#ifdef __cplusplus
}
#endif

#endif /* !RTSP_SDP_H */
