#ifndef __EASYRTOSQUEUE_H
#define __EASYRTOSQUEUE_H

typedef struct eQueue
{
    EASYRTOS_TCB *  putSuspQ;   /* �ȴ��������ݷ��͵�������� */
    EASYRTOS_TCB *  getSuspQ;   /* �ȴ��������ݽ��յ�������� */
    void *   buff_ptr;          /* ��������ָ�� */
    uint32_t    unit_size;      /* ������Ϣ�Ĵ�С */
    uint32_t    max_num_msgs;   /* ����Ϣ���� */
    uint32_t    insert_index;   /* ��Ϣ�������� */
    uint32_t    remove_index;   /* ��Ϣ�Ƴ����� */
    uint32_t    num_msgs_stored;/* ������Ϣ���� */
} EASYRTOS_QUEUE;

typedef struct eQueuetimer
{
    EASYRTOS_TCB   *tcb_ptr;    /* Thread which is suspended with timeout */
    EASYRTOS_QUEUE *queue_ptr;  /* Queue the thread is interested in */
    EASYRTOS_TCB   **suspQ;     /* TCB queue which thread is suspended on */
} QUEUE_TIMER;

/* ȫ�ֺ��� */
extern EASYRTOS_QUEUE eQueueCreate ( void *buff_ptr, uint32_t unit_size, uint32_t max_num_msgs);
extern ERESULT eQueueDelete (EASYRTOS_QUEUE *qptr);
extern ERESULT eQueueTake (EASYRTOS_QUEUE *qptr, int32_t timeout, void *msgptr);
extern ERESULT eQueueGive (EASYRTOS_QUEUE *qptr, int32_t timeout, void *msgptr);

#endif
