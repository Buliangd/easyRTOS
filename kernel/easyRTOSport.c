/**
 * ����: Roy.yu
 * ʱ��: 2017.04.26
 * �汾: V1.2
 * Licence: GNU GENERAL PUBLIC LICENSE
 */
#include "easyRTOS.h"
#include "easyRTOSkernel.h"
#include "easyRTOSport.h"
#include "easyRTOSTimer.h"
#include "stm8s_tim3.h"

/* ȫ�ֺ��� */
void archTaskContextInit (EASYRTOS_TCB *tcb_ptr, void *stack_top, void (*entry_point)(uint32_t), uint32_t entryParam);
void archInitSystemTickTimer ( void );
/* end */

/* ˽�к��� */
static void taskShell (void);
/* end */

/**
 * ����: ������ں���,��tcb.entry_point�������ݸ�������,
 * �����õ�ǰ����������,ʹ���ж�.
 *
 * ����:
 * ����:                         ���:
 * ��                            ��.
 *
 * ����: void
 * 
 * ���õĺ���:
 * eCurrentContext();
 * rim();
 * curr_tcb->entry_point(curr_tcb->entryParam);
 */
static void taskShell (void)
{
    EASYRTOS_TCB *curr_tcb;

    curr_tcb = eCurrentContext();

    /* ʹ���ж� */
    rim();

    /* ��������� */
    if (curr_tcb && curr_tcb->entry_point)
    {
        curr_tcb->entry_point(curr_tcb->entryParam);
    }
}

/**
 * ����: ��ʼ����������Ķ�ջ,ȷ������Ĵ���?b8-?b15��λ��,
 * �Լ�������ڵ�ַ,�������RETʹ��.
 *
 * ����:
 * ����:                         ���:
 * ��                            ��.
 *
 * ����: void
 * 
 * ���õĺ���:
 * eCurrentContext();
 * rim();
 * curr_tcb->entry_point(curr_tcb->entryParam);
 */
void archTaskContextInit (EASYRTOS_TCB *tcb_ptr, void *stack_top, void (*entry_point)(uint32_t), uint32_t entryParam)
{
    uint8_t *stack_ptr;
    stack_ptr = (uint8_t *)stack_top;
    /**
     *  ����ָ������ִ�� RET ָ��.��ָ����Ҫ�ڶ�ջ���ҵ����л�������ĳ����ַ.
     *  ��һ�������һ������ʱ,�᷵���������.���ǽ�������ڴ洢�ڶ�ջ��,ͬʱRET
     *  Ҳ���ڶ�ջ��Ѱ��������򷵻ص�ַ.

     *  ������ taskShell() �����������е�����,����ʵ�������ǽ�taskShell()��
     *  ��ַ�洢������.

     *  ���ϵ���ʹ�ö�ջ.
     */
    *stack_ptr-- = (uint8_t)((uint16_t)taskShell & 0xFF);
    *stack_ptr-- = (uint8_t)(((uint16_t)taskShell >> 8) & 0xFF);

    /* ʹ��IAR��ʱ�����һЩ����Ĵ���,��ʼ����Щ����Ĵ�����λ�� */
    *stack_ptr-- = 0;    // ?b8
    *stack_ptr-- = 0;    // ?b9
    *stack_ptr-- = 0;    // ?b10
    *stack_ptr-- = 0;    // ?b11
    *stack_ptr-- = 0;    // ?b12
    *stack_ptr-- = 0;    // ?b13
    *stack_ptr-- = 0;    // ?b14
    *stack_ptr-- = 0;    // ?b15

    /**
     *  �������������йص����ݶ��ѳ�ʼ�����.ʣ�µľ�����TCB�б���ջָ��,��������
     *  ʹ��.
     */
    tcb_ptr->sp_save_ptr = stack_ptr;
}

/**
 * ����: ��ʼ��ϵͳ����ʹ�õĶ�ʱ��,����ʹ�õ���TIM3,Ҳ����ʹ��������ʱ��
 * Ƶ����SYSTEM_TICKS_HZ����,��Ƶ�ʼ�Ϊϵͳ�Զ��л������ʱ��.
 *
 * ����:
 * ����:                         ���:
 * ��                            ��.
 *
 * ����: void
 * 
 * ���õĺ���:
 * TIM4_DeInit();
 * TIM4_TimeBaseInit(TIM3_PRESCALER_16, (uint16_t)((uint32_t)1000000/SYSTEM_TICKS_HZ));
 * TIM4_ITConfig(TIM3_IT_UPDATE, ENABLE);
 * TIM4_Cmd(ENABLE);
 */
void archInitSystemTickTimer ( void )
{
    /* ��ʼ��TIM3*/
    TIM4_DeInit();

    /* ����ϵͳ���� */
    TIM4_TimeBaseInit(TIM4_PRESCALER_128, (uint16_t)((uint32_t)125000/SYSTEM_TICKS_HZ));

    TIM4_ITConfig(TIM4_IT_UPDATE, ENABLE);

    /* ʹ��TIM3 */
    TIM4_Cmd(ENABLE);
}

/**
 * ����: ϵͳ����ʱ���жϳ���,���������ж�ʱ����count,��������Ҫ����
 * �Ķ�ʱ���ص�,�����˳���ʱ����õ�����.
 *
 * ����:
 * ����:                         ���:
 * ��                            ��.
 *
 * ����: void
 * 
 * ���õĺ���:
 * eIntEnter ();
 * eTimerTick();
 * eIntExit (TRUE);
 */
/* IAR���ж����� */
#pragma vector = ITC_IRQ_TIM4_OVF + 2
__interrupt  void TIM3_SystemTickISR (void)
{
    eIntEnter ();
    
    eTimerTick();

    TIM4->SR1 = (uint8_t)(~(uint8_t)TIM4_IT_UPDATE);
    
    eIntExit (TRUE);
}
