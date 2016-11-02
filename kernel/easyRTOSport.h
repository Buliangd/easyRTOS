/**
 * ����: Roy.yu
 * ʱ��: 2016.11.01
 * �汾: V1.1
 * Licence: GNU GENERAL PUBLIC LICENSE
 */
#ifndef __EASYRTOSPORT__H__
#define __EASYRTOSPORT__H__

#include "intrinsics.h"
#include "stddef.h"

#define CRITICAL_STORE      __istate_t _istate
#define CRITICAL_ENTER()    _istate = __get_interrupt_state(); __disable_interrupt()
#define CRITICAL_EXIT()     __set_interrupt_state(_istate)

/* ϵͳ����Ƶ�� */
#define SYSTEM_TICKS_HZ                 100

/* ��ʱʱ��ת������λms min(N) = 1000/SYSTEM_TICKS_HZ INT */
#define DELAY_MS(x) ((x)*(SYSTEM_TICKS_HZ)/(1000))

/* ��ʱʱ��ת������λs min(N) = 1/SYSTEM_TICKS_HZ INT */
#define DELAY_S(x) ((x)*(SYSTEM_TICKS_HZ))

/* ��ջ��λ��С stm8Ϊ8bit */
#define STACK_ALIGN_SIZE                sizeof(u8)

extern void archTaskContextInit (EASYRTOS_TCB *tcb_ptr, void *stack_top, void (*entry_point)(uint32_t), uint32_t entryParam);
extern void archInitSystemTickTimer ( void );
#endif
