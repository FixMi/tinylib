
/* time wheel������ʵ����ִ�ж��ٲ�(time_wheel_step)֮��ִ���û�ָ���ĺ���
 * ������ʵ�ֶ�ʱ���������ص��ǿ����ڳ���ʱ���ڼ������Ԫ�ĳ�ʱ�¼�
 *  ��ͳ��Ϊtimer
 */

#ifndef UTIL_TIME_WHEEL_H
#define UTIL_TIME_WHEEL_H

struct time_wheel;
typedef struct time_wheel time_wheel_t;

typedef enum{
    TIME_WHEEL_EXPIRE_ONESHOT,
    TIME_WHEEL_EXPIRE_LOOP,
}time_wheel_expire_type_e;

#ifdef __cplusplus
extern "C" {
#endif 

/* �������ֵ��oneshot�����timer��һ���Եģ���ʱ֮���ٻ
 * ��֮����ֵ������ֵʱʱĬ��Ϊloop����timer��ѭ��timer��ֱ���䷵��oneshot��cancelΪֹ
 */
typedef int(*onexpire_f)(void *userdata);

time_wheel_t* time_wheel_create(unsigned max_step);

void time_wheel_destroy(time_wheel_t* wheel);

/* ����ֵΪtimer��handle����time_wheel_refreshʱʹ�� */
void* time_wheel_submit(time_wheel_t* wheel, onexpire_f func, void* userdata, unsigned steps);

void time_wheel_cancel(time_wheel_t* wheel, void *handle);

/* ����handleָ����timer��ʹ�����ִ��һ�� */
void time_wheel_refresh(time_wheel_t* wheel, void *handle);

void time_wheel_step(time_wheel_t* wheel);

#ifdef __cplusplus
}
#endif

#endif /* UTIL_TIME_WHEEL_H */

