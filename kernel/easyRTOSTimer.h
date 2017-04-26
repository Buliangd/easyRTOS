/**
 * ����: Roy.yu
 * ʱ��: 2017.04.26
 * �汾: V1.2
 * Licence: GNU GENERAL PUBLIC LICENSE
 */

#ifndef __EASYRTOSTIMER_H__
#define __EASYRTOSTIMER_H__
/* Delay �ص��ṹ�� */
typedef struct delay_timer
{
  EASYRTOS_TCB *tcb_ptr;  /* ��Delay�������б� */
}DELAY_TIMER;
  
/* ȫ�ֺ��� */
extern void eTimerTick (void);
extern ERESULT eTimerDelay (uint32_t ticks);
extern void eTimerCallbacks (void);
extern ERESULT eTimerRegister (EASYRTOS_TIMER *timer_ptr);
extern ERESULT eTimerCancel (EASYRTOS_TIMER *timer_ptr);
extern uint32_t eTimeGet(void);
extern void eTimeSet(uint32_t newTime);
#endif
