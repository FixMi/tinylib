
#ifndef NET_LOOP_H
#define NET_LOOP_H

struct loop;
typedef struct loop loop_t;

#include "net/timer.h"
#include "net/channel.h"

#ifdef __cplusplus
extern "C" {
#endif

loop_t* loop_new(unsigned hint);

void loop_destroy(loop_t *loop);

int loop_update_channel(loop_t *loop, channel_t* channel);

/* �ύһ���첽ִ�����񣬸÷������̰߳�ȫ�� */
/* �벻ҪƵ�����ύ�첽���񣬷����ʹ���ں�ִ̬��ʱ���������� */
void loop_async(loop_t* loop, void(*callback)(void *userdata), void* userdata);

/* ��ָ��ѭ����ִ��һ���ص��������÷������̰߳�ȫ��
 * �벻ҪƵ���ؿ��߳�ʹ�øýӿڣ������ʹ���ں�ִ̬��ʱ����������
 */
void loop_run_inloop(loop_t* loop, void(*callback)(void *userdata), void* userdata);

void loop_loop(loop_t* loop);

void loop_quit(loop_t* loop);

/* ������ʱ������msΪ��λ */
/* һ����timer����ʱ֮��ᱻ�Զ����գ������ֶ�cancel */
loop_timer_t* loop_runafter(loop_t* loop, unsigned interval, onexpire_f expirecb, void *userdata);
/* ������timer */
loop_timer_t* loop_runevery(loop_t* loop, unsigned interval, onexpire_f expirecb, void *userdata);

void loop_cancel(loop_t* loop, loop_timer_t *timer);
/* ����loop_runevery���ص�ѭ��timer��Ч */
void loop_refresh(loop_t* loop, loop_timer_t *timer);

#ifdef __cplusplus
}
#endif

#endif /* NET_LOOP_H */

