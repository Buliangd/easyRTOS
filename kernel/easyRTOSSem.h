#ifndef __EASYRTOSSEM_H
#define __EASYRTOSSEM_H

/* �ź������� */
#define SEM_BINARY  0x01
#define SEM_MUTEX   0x02
#define SEM_COUNTY  0x03

typedef struct easyRTOSSem
{
    EASYRTOS_TCB * suspQ;  /* ��Sem������������� */
    EASYRTOS_TCB * owner;  /* ��MUTEX��ס������ */
    int8_t         count;  /* �ź������� -127 - 127*/
    uint8_t        type;   /* �ź������� */
} EASYRTOS_SEM;

typedef struct easyRTOSSemTimer
{
    EASYRTOS_TCB *tcb_ptr;  /* ����timeout���������� */
    EASYRTOS_SEM *sem_ptr;  /* ����������ź��� */
} SEM_TIMER;

/* ȫ�ֺ��� */
extern EASYRTOS_SEM eSemCreateBinary ();
extern EASYRTOS_SEM eSemCreateCount  (uint8_t initial_count);
extern EASYRTOS_SEM eSemCreateMutex  ();
extern ERESULT eSemDelete (EASYRTOS_SEM *sem);
extern ERESULT eSemTake (EASYRTOS_SEM *sem, int32_t timeout);
extern ERESULT eSemGive (EASYRTOS_SEM *sem);
extern ERESULT eSemResetCount (EASYRTOS_SEM *sem, uint8_t count);

#endif
