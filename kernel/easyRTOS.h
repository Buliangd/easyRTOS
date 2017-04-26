/**
 * ����: Roy.yu
 * ʱ��: 2017.04.26
 * �汾: V1.2
 * Licence: GNU GENERAL PUBLIC LICENSE
 */
#ifndef __EASYRTOS__H__
#define __EASYRTOS__H__

#include "stm8s.h"
/* Constants */
#define TRUE                    1
#define FALSE                   0
#ifndef NULL
  #define NULL 0
#endif
/* End */

#define POINTER       void *
#define TASKNAMELEN   10
#define TASKSTATE     uint8_t

#define TASK_RUN      0x01    /*����*/
#define TASK_READY    0x02    /*����*/
#define TASK_PENDED   0x04    /*����*/
#define TASK_DELAY    0x08    /*�ӳ�*/
#define TASK_SUSPEND  0x10    /*����*/

typedef void ( * TIMER_CB_FUNC ) ( POINTER cb_data ) ;

typedef struct easyRTOS_timer
{
  TIMER_CB_FUNC   cb_func;    /* �ص����� */
  POINTER	        cb_data;    /* �ص������Ĳ���ָ�� */
  uint32_t	      cb_ticks;   /* ��ʱ��count���� */

	/* �ڲ����� */
  struct easyRTOS_timer *next_timer;		/* ˫������ */
} EASYRTOS_TIMER;

typedef struct easyRTOS_tcb
{
    /* ����ջָ��.�����񱻵������л���ʱ��,ջָ�뱣������������� */
    POINTER sp_save_ptr;

    /* �̵߳����ȼ� (0-255) */
    uint8_t priority;

    /**
     *  ����������Լ�����.
     */
    void (*entry_point)(uint32_t);
    uint32_t entryParam;

    /* �������ָ������ */
    struct easyRTOS_tcb *prev_tcb;    /* ָ��ǰһ��TCB��˫��TCB����ָ��*/
    struct easyRTOS_tcb *next_tcb;    /* ָ���һ��TCB��˫��TCB����ָ��*/

    /**
     *  ����״̬:
     *  ����
     *  �ӳ�
     *  ����
     *  ����
     *  ����
     */
    TASKSTATE state;
    int8_t pendedWakeStatus;        
    EASYRTOS_TIMER *pended_timo_cb;  
    EASYRTOS_TIMER *delay_timo_cb;   

    /* ����ID */
    uint8_t taskID;

    /* �������� */
    uint8_t taskName[TASKNAMELEN];

    /* �����ڴ���֮���ʵ������̬��ʱ��,����CPUʹ����ʱʹ�� */
    uint32_t taskRunTime;
} EASYRTOS_TCB;

extern void archContextSwitch (EASYRTOS_TCB *old_tcb_ptr, EASYRTOS_TCB *new_tcb_ptr);
extern void archFirstTaskRestore (EASYRTOS_TCB *new_tcb_ptr);
#endif
