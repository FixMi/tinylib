
/** ��rtsp��Ϣ���н��������ɶ�Ӧ��request/response���� 
  * �����򵥽���������head��value�ֶΣ����ڸ���ʹ��ʱ������һ������
  */

#ifndef TINYLIB_RTSP_MESSAGE_CODEC_H
#define TINYLIB_RTSP_MESSAGE_CODEC_H

typedef enum rtsp_method {
    RTSP_METHOD_NONE = 0,
    RTSP_METHOD_OPTIONS = 1,
    RTSP_METHOD_DESCRIBE = 1<<1,
    RTSP_METHOD_ANNOUNCE = 1<<2,
    RTSP_METHOD_SETUP = 1<<3,
    RTSP_METHOD_PLAY = 1<<4,
    RTSP_METHOD_PAUSE = 1<<5,
    RTSP_METHOD_TEARDOWN = 1<<6,
    RTSP_METHOD_GET_PARAMETER = 1<<7,
    RTSP_METHOD_SET_PARAMETER = 1<<8,
    RTSP_METHOD_REDIRECT = 1<<9,
    RTSP_METHOD_RECORD = 1<<10,
}rtsp_method_e;

typedef enum rtsp_head_key{
    RTSP_HEAD_ACCEPT,
    RTSP_HEAD_ACCEPT_ENCODING,
    RTSP_HEAD_ACCEPT_LANGUAGE,
    RTSP_HEAD_ALLOW,
    RTSP_HEAD_AUTHORIZATION,
    RTSP_HEAD_BANDWIDTH,
    RTSP_HEAD_BLOCKSIZE,
    RTSP_HEAD_CACHE_CONTROL,
    RTSP_HEAD_CONFERENCE,
    RTSP_HEAD_CONNECTION,
    RTSP_HEAD_CONTENT_BASE,
    RTSP_HEAD_CONTENT_ENCODING,
    RTSP_HEAD_CONTENT_LANGUAGE,
    RTSP_HEAD_CONTENT_LENGTH,
    RTSP_HEAD_CONTENT_LOCATION,
    RTSP_HEAD_CONTENT_TYPE,
    RTSP_HEAD_CSEQ,
    RTSP_HEAD_DATE,
    RTSP_HEAD_EXPIRES,
    RTSP_HEAD_FROM,
    RTSP_HEAD_HOST,
    RTSP_HEAD_IF_MATCH,
    RTSP_HEAD_IF_MODIFIED_SINCE,
    RTSP_HEAD_LAST_MODIFIED,
    RTSP_HEAD_LOCATION,
    RTSP_HEAD_PROXY_AUTHENTICATE,
    RTSP_HEAD_PROXY_REQUIRE,
    RTSP_HEAD_PUBLIC,
    RTSP_HEAD_RANGE,
    RTSP_HEAD_REFERER,
    RTSP_HEAD_RETRY_AFTER,
    RTSP_HEAD_REQUIRE,
    RTSP_HEAD_RTP_INFO,
    RTSP_HEAD_SCALE,
    RTSP_HEAD_SPEED,
    RTSP_HEAD_SERVER,
    RTSP_HEAD_SESSION,
    RTSP_HEAD_TIMESTAMP,
    RTSP_HEAD_TRANSPORT,
    RTSP_HEAD_UNSUPPORTED,
    RTSP_HEAD_USER_AGENT,
    RTSP_HEAD_VARY,
    RTSP_HEAD_VIA,
    RTSP_HEAD_WWW_AUTHENTICA
}rtsp_head_key_e;

typedef struct rtsp_head
{
    rtsp_head_key_e key;
    char* value;
    struct rtsp_head* next;
}rtsp_head_t;

typedef struct rtsp_request_msg
{
    rtsp_method_e method;       /* rtsp_method_e */
    char* url;
    int version;                /* 0x0100��ʾRTSP/1.0 */
    int cseq;                   /* ��������Ϣ�ĵ�cseqͷ�����������ʹ�� */
    rtsp_head_t* head;            /* ��CSeq֮���head */
    char *body;
    int body_len;
}rtsp_request_msg_t;

typedef struct rtsp_response_msg
{
    int version;                /* 0x0100��ʾRTSP/1.0 */
    int code;                   /* ��Ӧ����е�status code */
    int cseq;                   /* ��Ӧ��request��Ϣ�е�cseqͷ��Ϊ������������� */
    rtsp_head_t* head;          /* ��CSeq֮���head */
    char* body;                 /* ��Ϣ������ ��һ���Ӧ��DESCRIBE��Ӧ�е�SDP��Ϣ */
    int body_len;                /*��Ϣ��ĳ��ȣ�body����ʱ��Ч*/
}rtsp_response_msg_t;

typedef struct rtsp_interleaved_head{
    unsigned char magic;
    unsigned char channel;
    unsigned short len;
}rtsp_interleaved_head_t;

typedef struct rtsp_transport_head
{
    const char *trans;
    const char* cast;
    const char* destination;
    const char* source;
    unsigned short client_rtp_port;
    unsigned short client_rtcp_port;
    unsigned short server_rtp_port;
    unsigned short server_rtcp_port;
    const char* ssrc;
    unsigned interleaved;
    unsigned char rtp_channel;
    unsigned char rtcp_channel;
}rtsp_transport_head_t;

typedef struct rtsp_authenticate_head
{
    const char *type;
    const char *realm;
    const char *nonce;
    const char *stale;
}rtsp_authenticate_head_t;

#ifdef __cplusplus
extern "C" {
#endif

/* ����һ���յ�rtsp������Ϣ���� */
rtsp_request_msg_t* rtsp_request_msg_new(void);

/* ����һ��������������Ϣ���� */
void rtsp_request_msg_destroy(rtsp_request_msg_t* request_msg);

/* ������expat��ʽ��ʽ������н���
 * ����0 ��ʾ�ɹ�������һ��������rtsp������Ϣ
 * ����1 ��ʾ��������û�д��󣬵������ݲ���������rtsp������Ϣ��δ�������
 * ����-1 ��ʾ�������̳������������Ƿ�����Ϣ
 * 
 * parsed_bytes ��ʾ�ɹ�������һ��rtsp������Ϣʱ��������ֵΪ0ʱ���������˶����ֽڣ�������������������������
 */
int rtsp_request_msg_decode(rtsp_request_msg_t* request_msg, const char *data, int size, int *parsed_bytes);

/* ��������Ϣ����ԭ���Ե����ü�����������Ҫ�ڿ��̵߳Ļ�����ʹ�� */
/** ����һ�����ü��� */
void rtsp_request_msg_ref(rtsp_request_msg_t* request_msg);
/** ����һ�����ü����������ز�����ļ���ֵ��������ֵΪ0ʱ���ö��󽫱����� */
int rtsp_request_msg_unref(rtsp_request_msg_t* request_msg);

rtsp_response_msg_t* rtsp_response_msg_new(void);

/* ������expat��ʽ��ʽ������н���
 * ����0 ��ʾ�ɹ�������һ��������rtsp������Ϣ
 * ����1 ��ʾ��������û�д��󣬵������ݲ���������rtsp������Ϣ��δ�������
 * ����-1 ��ʾ�������̳������������Ƿ�����Ϣ
 * 
 * parsed_bytes ��ʾ�ɹ�������һ��rtsp������Ϣʱ��������ֵΪ0ʱ���������˶����ֽڣ�������������������������
 */
int rtsp_response_msg_decode(rtsp_response_msg_t* response_msg, const char *data, int size, int *parsed_bytes);

void rtsp_response_msg_destroy(rtsp_response_msg_t* response_msg);

/* ����Ӧ��Ϣ����ԭ���Ե����ü�����������Ҫ�ڿ��̵߳Ļ�����ʹ�� */
/** ����һ�����ü��� */
void rtsp_response_msg_ref(rtsp_response_msg_t* response_msg);
/** ����һ�����ü����������ز�����ļ���ֵ��������ֵΪ0ʱ���ö��󽫱����� */
int rtsp_response_msg_unref(rtsp_response_msg_t* response_msg);

/** ������Ӧ��Ϣ������time-header��cseq-header �ڲ���������䣬�����header��headָ�� */
int rtsp_msg_build_response
(
    char *response_msg, int len, int cseq, int code,
    rtsp_head_t* head, const char* body, int body_len
);

int rtsp_msg_buid_request
(
    char* request_msg, int len, int cseq, rtsp_method_e method, const char* url, 
    rtsp_head_t* head, const char* body, int body_len
);

rtsp_transport_head_t* rtsp_transport_head_decode(const char *transport);
void rtsp_transport_head_destroy(rtsp_transport_head_t* head);

rtsp_authenticate_head_t* rtsp_authenticate_head_decode(const char *auth);
void rtsp_authenticate_head_destroy(rtsp_authenticate_head_t* head);

/* ���ݸ�����user, password, method, url, realm, nonce����response, ����֯authorization��Ϣ */
int rtsp_authorization_head
(
    char *auth, int len, const char *user, const char *password, 
    const char *method, const char *url, const char *realm, const char *nonce
);

#ifdef __cplusplus
}
#endif

#endif /* !TINYLIB_RTSP_MESSAGE_CODEC_H */
