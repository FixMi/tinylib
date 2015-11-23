
/** buffer����ӿ� */

#ifndef NET_BUFFER_H
#define NET_BUFFER_H

struct buffer;
typedef struct buffer buffer_t;

#include <winsock2.h>

#ifdef __cplusplus
extern "C" {
#endif

/** ��ָ���ߴ紴����buffer */
buffer_t* buffer_new(unsigned size);

/** �ͷŸ����Ķ���֮�������ʹ��buffer_init()���³�ʼ��  */
void buffer_destory(buffer_t* buffer);

/** ��ȡ����buffer����Ч���ݵ���ʼ��ַ  */
void* buffer_peek(buffer_t* buffer);

/** ��ȡ����buffer����Ч���ݵĳߴ� */
unsigned buffer_readablebytes(buffer_t* buffer);

/** �������buffer��׷������  ��������ر���д������ݳߴ�*/
unsigned buffer_append(buffer_t* buffer, const void* data, unsigned size);

/** �Ӹ�����fd�ж�ȡ���ݵ�buffer�� ��������ر��ζ�ȡ�������ݳߴ�*/
unsigned buffer_readFd(buffer_t* buffer, SOCKET fd);

/**  ����ڸ�����buffer���Ѿ���ȡ����ָ�ߴ�����ݣ��ͷŶ�Ӧ�Ŀռ� */
void buffer_retrieve(buffer_t *buffer, unsigned size);

void buffer_retrieveall(buffer_t *buffer);

#ifdef __cplusplus
}
#endif

#endif /* !NET_BUFFER_H */

