
#ifndef UDP_PEER_H
#define UDP_PEER_H

struct udp_peer;
typedef struct udp_peer udp_peer_t;

#include "tinylib/windows/net/loop.h"
#include "tinylib/windows/net/inetaddr.h"

#include <winsock2.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*on_message_f)(udp_peer_t *peer, void *message, unsigned size, void* userdata, const inetaddr_t *peer_addr);

typedef void (*on_writable_f)(udp_peer_t *peer, void* userdata);

udp_peer_t* udp_peer_new(loop_t *loop, const char *ip, unsigned short port, on_message_f messagecb, on_writable_f writecb, void *userdata);

unsigned short udp_peer_getport(udp_peer_t* peer);

/* �ҽ�read�¼���on_message_fΪNULLʱ����ʾ���read�¼�������ԭ����on_message_f */
on_message_f udp_peer_onmessage(udp_peer_t* peer, on_message_f messagecb, void *userdata);

/* �ҽ�write�¼���writecbΪNULLʱ����ʾ���write�¼�������ԭ����wirtecb */
on_writable_f udp_peer_onwrite(udp_peer_t* peer, on_writable_f writecb, void *userdata);

void udp_peer_destroy(udp_peer_t* peer);

/* ����udp�ļ��ԣ�ֻ���򵥷��ͣ���ʹ����������ɱ��ķ�Ƭ����֤ÿ����Ϣ�ߴ粻����65535 */
int udp_peer_send(udp_peer_t* peer, const void *message, unsigned len, const inetaddr_t *peer_addr);
int udp_peer_send2(udp_peer_t* peer, const void *message, unsigned len, const struct sockaddr_in *peer_addr);

/* ��������buffer�ߴ磬ÿ����1KΪ��λ����Բ�� */
void udp_peer_expand_send_buffer(udp_peer_t* peer, unsigned size);
void udp_peer_expand_recv_buffer(udp_peer_t* peer, unsigned size);

#ifdef __cplusplus
}
#endif

#endif /* !UDP_PEER_H */
