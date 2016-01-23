
#ifndef RTP_PEER_H
#define RTP_PEER_H

#ifdef OS_WINDOWS
	#include "tinylib/windows/net/udp_peer.h"
#else
	#include "tinylib/linux/net/udp_peer.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct rtp_peer;
typedef struct rtp_peer rtp_peer_t;

/* ��ʼ��һ��rtp_peer��
 * start_port ָ����ʹ�õļ����˿ڵ���ʼֵ
 * peer_count ָ������ж��ٸ�rtp_peer��һ��rtp_peerͬʱ����rtp/rtcp�˵�
 * �ʶ�ʵ����ཫռ��peer_count * 2��UDP�˿�
 */
void rtp_peer_pool_init(unsigned short start_port, unsigned peer_count);
void rtp_peer_pool_uninit(void);

/* �ڸ�����ip�Ϸ���һ��rtp_peer��rtp/rtcp�˿���ż�����ڵ�
 * rtpwritecb/rtcpwritecbΪNULLʱ��ʾ����֪��Ӧ��Ϣ��write�¼�����Ҫʱ������udp_peer_onwrite()���н��йҽ�
 */
rtp_peer_t* rtp_peer_alloc
(
	loop_t* loop, const char *ip, 
	on_message_f rtpcb, on_writable_f rtpwritecb, 
	on_message_f rtcpcb, on_writable_f rtcpwritecb, void* userdata
);

void rtp_peer_free(rtp_peer_t* peer);

unsigned short rtp_peer_rtpport(rtp_peer_t* peer);

unsigned short rtp_peer_rtcpport(rtp_peer_t* peer);

udp_peer_t* rtp_peer_get_rtp_udppeer(rtp_peer_t* peer);

udp_peer_t* rtp_peer_get_rtcp_udppeer(rtp_peer_t* peer);

/* ��ָ���ĵ�ַ����һ��RTCP BYE��Ϣ */
void rtp_peer_bye(rtp_peer_t* peer, const inetaddr_t *peer_addr);

#ifdef __cplusplus
}
#endif

#endif /* !RTP_PEER_H */
