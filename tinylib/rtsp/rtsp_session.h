
#ifndef RTSP_SESSION_H
#define RTSP_SESSION_H

struct rtsp_session;
typedef struct rtsp_session rtsp_session_t;

#ifdef WINNT
	#include "tinylib/windows/net/tcp_connection.h"
#elif defined(__linux__)
	#include "tinylib/linux/net/tcp_connection.h"
#endif

#include "tinylib/rtsp/rtsp_message_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

/* �Ự�½��������ӶϿ�ʱ�� request_msg ΪNULL, ʹ�� tcp_connection_connected() �������������
 * request_msg����Ӧ��������Ϣ����
 */
typedef void (*rtsp_session_handler_f)
(
	rtsp_session_t* session, 
	tcp_connection_t* connection, 
	rtsp_request_msg_t *request_msg, 
	void *userdata
);

typedef void (*rtsp_session_interleaved_packet_f)
(
	rtsp_session_t* session, 
	unsigned char channel,
	void* packet, unsigned short size,
	void *userdata
);

rtsp_session_t* rtsp_session_start
(
	tcp_connection_t *connection, 
	rtsp_session_handler_f session_handler, 
	rtsp_session_interleaved_packet_f interleaved_sink, 
	void* userdata
);

void rtsp_session_set_extra_userdata(rtsp_session_t* session, void *userdata);
void* rtsp_session_get_extra_userdata(rtsp_session_t* session);

void rtsp_session_end(rtsp_session_t* session);

#ifdef __cplusplus
}
#endif

#endif /* !RTSP_SESSION_H */

