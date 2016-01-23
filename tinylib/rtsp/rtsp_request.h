
/** ʵ��һ������rtsp�ͻ��ˣ���֧��pipeline��ʽ�������ͺ�Ӧ���� */

#ifndef RTSP_REQUEST_H
#define RTSP_REQUEST_H

struct rtsp_request;
typedef struct rtsp_request rtsp_request_t;

#if WINNT
	#include "tinylib/windows/net/tcp_connection.h"
#elif defined(__linux__)
	#include "tinylib/linux/net/tcp_connection.h"
#endif

#include "tinylib/rtsp/rtsp_message_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

/** rtsp������Ӧ��Ϣ�Ĵ���ص�
  *
  * @connnection: ��Ӧ���λỰ��tcp����
  * @method: �յ�����Ӧ��Ϣ֮ǰ��client�˷���������Ϣ�е����󷽷���
  *          �����RTSP_METHOD_NONE��ʾ��Զ��RTSP�����������ӽ���OK����ʱresponseΪNULL
  * @respons: ��Ӧ��Ϣ��������body����
  */
typedef void (*rtsp_request_handler_f)
(
	rtsp_request_t *request, 
	tcp_connection_t* connection, 
	rtsp_method_e method, 
	const rtsp_response_msg_t* response, 
	void *userdata
);

typedef void (*rtsp_request_interleaved_packet_f)
(
	rtsp_request_t *request, 
	unsigned char channel,
	void* packet, unsigned short size,
	void *userdata
);

rtsp_request_t* rtsp_request_new
(
	loop_t* loop, 
	const char* url, 
	rtsp_request_handler_f handler, 
	rtsp_request_interleaved_packet_f interleaved_sink, 
	void* userdata
);

void rtsp_request_destroy(rtsp_request_t* request);

int rtsp_request_launch(rtsp_request_t* request);

/* ����Զ�˷�����֧�ֵķ���������OPTIONS������ȷ��Ӧ֮�����Ч����������0 */
unsigned rtsp_request_server_method(rtsp_request_t* request);

/* ��ȡ���������صĻػ���ʱֵ */
int rtsp_request_timeout(rtsp_request_t* request);

int rtsp_request_options(rtsp_request_t* request);

int rtsp_request_describe(rtsp_request_t* request, rtsp_head_t* head);

/* �����url����ָ������Ϊ�ĸ�SDP�е���ý��������SETUP��
  * ��ΪNULL����Ĭ��ʹ�����󴴽�ʱָ����URL
  */
int rtsp_request_setup(rtsp_request_t* request, rtsp_head_t* head, const char *url);

int rtsp_request_play(rtsp_request_t* request, rtsp_head_t* head);

int rtsp_request_teardown(rtsp_request_t* request, rtsp_head_t* head);

int rtsp_request_get_parameter(rtsp_request_t* request, rtsp_head_t* head, const char* body, unsigned body_len);

int rtsp_request_set_parameter(rtsp_request_t* request, rtsp_head_t* head, const char* body, unsigned body_len);

#ifdef __cplusplus
}
#endif

#endif /*  !RTSP_REQUEST_H */

