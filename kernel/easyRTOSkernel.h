/**
 * ����: Roy.yu
 * ʱ��: 2017.04.26
 * �汾: V1.2
 * Licence: GNU GENERAL PUBLIC LICENSE
 */
#ifndef __EASYRTOSKERNEL_H__
#define __EASYRTOSKERNEL_H__

#include "easyRTOS.h"
#define ERESULT int8_t
#define EASYRTOS_OK            ( 0) /* �ɹ� */
#define EASYRTOS_ERR_PARAM     (-1) /* ����Ĳ��� */
#define EASYRTOS_ERR_QUEUE     (-2) /* ���������Ready����ʧ�� */
#define EASYRTOS_ERR_NOT_FOUND (-3) /* û���ҵ�ע���timer */
#define EASYRTOS_ERR_CONTEXT   (-4) /* ����������ĵ��� */
#define EASYRTOS_ERR_TIMER     (-5) /* ע�ᶨʱ��ʧ��/ȡ����ʱ��ʧ��*/
#define EASYRTOS_ERR_DELETED   (-6) /* ����������ʱ��ɾ�� */
#define EASYRTOS_TIMEOUT       (-7) /* �ź���timeout���� */
#define EASYRTOS_ERR_OVF       (-8) /* �����ź���count>127����С�� -127 */
#define EASYRTOS_WOULDBLOCK    (-9) /* �����ᱻ���ҵ�����timeoutΪ-1���Է����� */
#define EASYRTOS_ERR_BIN_OVF   (-10)/* ��ֵ�ź���count�Ѿ�Ϊ1 */
#define EASYRTOS_SEM_UINIT     (-11)/* �ź���û�б���ʼ�� */
#define EASYRTOS_ERR_OWNERSHIP (-12)/* ���Խ����������������ǻ�����ӵ���� */

/* ȫ�ֺ��� */
extern ERESULT eTaskCreat(EASYRTOS_TCB *task_tcb, uint8_t priority, void (*entry_point)(uint32_t), uint32_t entryParam, void* taskStack, uint32_t stackSize,const char* taskName,uint32_t taskID);
extern void easyRTOSStart (void);
extern void easyRTOSSched (uint8_t timer_tick);
ERESULT easyRTOSInit (void *idle_task_stack, uint32_t idleTaskStackSize);
extern ERESULT tcbEnqueuePriority (EASYRTOS_TCB **tcb_queue_ptr, EASYRTOS_TCB *tcb_ptr);
extern EASYRTOS_TCB *eCurrentContext (void);
extern void eIntEnter (void);
extern void eIntExit (uint8_t timerTick);
extern EASYRTOS_TCB *tcb_dequeue_entry (EASYRTOS_TCB **tcb_queue_ptr, EASYRTOS_TCB *tcb_ptr);
extern EASYRTOS_TCB *tcb_dequeue_head (EASYRTOS_TCB **tcb_queue_ptr);
extern EASYRTOS_TCB *tcb_dequeue_priority (EASYRTOS_TCB **tcb_queue_ptr, uint8_t priority);
/* end */

/* ȫ�ֱ��� */
extern uint8_t easyRTOSStarted;
extern EASYRTOS_TCB *tcb_readyQ;

#endif