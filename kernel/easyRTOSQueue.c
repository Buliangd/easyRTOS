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
#include "easyRTOSQueue.h"

#include "string.h"

/* ˽�к��� */
static ERESULT queue_remove (EASYRTOS_QUEUE *qptr, void* msgptr);
static ERESULT queue_insert (EASYRTOS_QUEUE *qptr, void* msgptr);
static void eQueueTimerCallback (POINTER cb_data);

/* ȫ�ֺ��� */
EASYRTOS_QUEUE eQueueCreate ( void *buff_ptr, uint32_t unit_size, uint32_t max_num_msgs);
ERESULT eQueueDelete (EASYRTOS_QUEUE *qptr);
ERESULT eQueueTake (EASYRTOS_QUEUE *qptr, int32_t timeout, void *msgptr);
ERESULT eQueueGive (EASYRTOS_QUEUE *qptr, int32_t timeout, void *msgptr);

/**
 * ����: ���д���,��ʼ�����нṹ���ڵĲ���,������.
 *
 * ����:
 * ����:                                   ���:
 * void *buff_ptr ������Ϣ�����ַָ��      ��
 * uint32_t unit_size ������Ϣ��ռ�ֽ�
 * uint32_t max_num_msgs ����Ϣ����
 *
 * ����:
 * EASYRTOS_QUEUE
 * 
 * ���õĺ���:
 * ��.
 */
EASYRTOS_QUEUE eQueueCreate ( void *buff_ptr, uint32_t unit_size, uint32_t max_num_msgs)
{
  EASYRTOS_QUEUE qptr;

  /* �洢�������� */
  qptr.buff_ptr = buff_ptr;
  qptr.unit_size = unit_size;
  qptr.max_num_msgs = max_num_msgs;

  /* ��ʼ��������������� */
  qptr.putSuspQ = NULL;
  qptr.getSuspQ = NULL;

  /* ��ʼ������/�Ƴ����� */
  qptr.insert_index = 0;
  qptr.remove_index = 0;
  qptr.num_msgs_stored = 0;

  return (qptr);
}

/**
 * ����: ɾ������,���������б��ö������ҵ��������Ready�б���.ͬʱȡ��������
 * ע��Ķ�ʱ��.�������񱻻���,�������������.
 *
 * ����:
 * ����:                                   ���:
 * EASYRTOS_QUEUE *qptr ����ָ��           EASYRTOS_QUEUE *qptr ����ָ��
 *
 * ����:
 * EASYRTOS_OK �ɹ�
 * EASYRTOS_ERR_QUEUE ��������õ�Ready������ʧ��
 * EASYRTOS_ERR_TIMER ȡ����ʱ��ʧ��
 * 
 * ���õĺ���:
 * tcb_dequeue_head (&qptr->getSuspQ);
 * tcb_dequeue_head (&qptr->putSuspQ);
 * tcbEnqueuePriority (&tcb_readyQ, tcb_ptr);
 * eTimerCancel (tcb_ptr->pended_timo_cb);
 * eCurrentContext();
 * easyRTOSSched (FALSE);
 */
ERESULT eQueueDelete (EASYRTOS_QUEUE *qptr)
{
  ERESULT status;
  CRITICAL_STORE;
  EASYRTOS_TCB *tcb_ptr;
  uint8_t wokenTasks = FALSE;

  /* ������� */
  if (qptr == NULL)
  {
    status = EASYRTOS_ERR_PARAM;
  }
  else
  {
    /* Ĭ�Ϸ��� */
    status = EASYRTOS_OK;

    /* �������б����ҵ����񣨽������Ready���У� */
    while (1)
    {
      /* �����ٽ��� */
      CRITICAL_ENTER ();

      /* ����Ƿ����̱߳����� ���ȴ����ͻ�ȴ����գ� */
      if (((tcb_ptr = tcb_dequeue_head (&qptr->getSuspQ)) != NULL)
          || ((tcb_ptr = tcb_dequeue_head (&qptr->putSuspQ)) != NULL))
      {

        /* ���ش���״̬ */
        tcb_ptr->pendedWakeStatus = EASYRTOS_ERR_DELETED;

        /* ���������Ready���� */
        if (tcbEnqueuePriority (&tcb_readyQ, tcb_ptr) != EASYRTOS_OK)
        {
          /* �˳��ٽ��� */
          CRITICAL_EXIT ();

          /* �˳�ѭ�������ش��� */
          status = EASYRTOS_ERR_QUEUE;
          break;
        }
        else tcb_ptr->state = TASK_READY;

        /* ȡ����������ע��Ķ�ʱ�� */
        if (tcb_ptr->pended_timo_cb)
        {
          /* ȡ���ص����� */
          if (eTimerCancel (tcb_ptr->pended_timo_cb) != EASYRTOS_OK)
          {
              /* �˳��ٽ��� */
              CRITICAL_EXIT ();

              /* �˳�ѭ�������ش��� */
              status = EASYRTOS_ERR_TIMER;
              break;
          }

          /* �������û�ж�ʱ���ص� */
          tcb_ptr->pended_timo_cb = NULL;
        }

        /* �˳��ٽ��� */
        CRITICAL_EXIT ();

        /* �Ƿ���õ����� */
        wokenTasks= TRUE;
      }

      /* û�б����ҵ����� */
      else
      {
        /* �˳��ٽ��� */
        CRITICAL_EXIT ();
        break;
      }
    }

    /* �������񱻻��ѣ����õ����� */
    if (wokenTasks == TRUE)
    {
      /**
       * ֻ�����������Ļ������õ�������
       * �жϻ�������eIntExit()���õ�������
       */
      if (eCurrentContext())
        easyRTOSSched (FALSE);
    }
  }

  return (status);
}

/**
 * ����: ȡ�������е�����,���Զ���Ϊ��,����timeout�Ĳ�ֵͬ�в�ͬ�Ĵ���ʽ.
 * 1.timeout>0 ���ҵ��õ�����,��timeout���ڵ�ʱ�������񲢷���timeout��־
 * 2.timeout=0 �������ҵ��õ�����,ֱ���Ӷ����еȵ�����.
 * 3.timeout=-1 ����������,������Ϊ�ջ᷵�ض���Ϊ�ձ�־.
 * �����������ҵ�ʱ��,������õ�����.
 *
 * ����:
 * ����:                                        ���:
 * EASYRTOS_QUEUE *qptr ����ָ��                EASYRTOS_QUEUE *qptr ����ָ��
 * int32_t timeout timeoutʱ��,����������ʱ��   void *msgptr ��ȡ����Ϣ
 * void *msgptr ��ȡ����Ϣ                 
 * 
 * ����:
 * EASYRTOS_OK �ɹ�
 * EASYRTOS_TIMEOUT �ź���timeout����
 * EASYRTOS_WOULDBLOCK �����ᱻ���ҵ�����timeoutΪ-1���Է�����
 * EASYRTOS_ERR_DELETED ��������������ʱ��ɾ��
 * EASYRTOS_ERR_CONTEXT ����������ĵ���
 * EASYRTOS_ERR_PARAM ��������
 * EASYRTOS_ERR_QUEUE ������������ж���ʧ��
 * EASYRTOS_ERR_TIMER ע�ᶨʱ��δ�ɹ�
 * 
 * ���õĺ���:
 * eCurrentContext();
 * tcbEnqueuePriority (&qptr->getSuspQ, curr_tcb_ptr);
 * eTimerRegister (&timerCb);
 * tcb_dequeue_entry (&qptr->getSuspQ, curr_tcb_ptr);
 * easyRTOSSched (FALSE);
 * queue_remove (qptr, msgptr);
 */
ERESULT eQueueTake (EASYRTOS_QUEUE *qptr, int32_t timeout, void *msgptr)
{
    CRITICAL_STORE;
    ERESULT status;
    QUEUE_TIMER timerData;
    EASYRTOS_TIMER timerCb;
    EASYRTOS_TCB *curr_tcb_ptr;

    /* ������� */
    if ((qptr == NULL)) //|| (msgptr == NULL))
    {
      status = EASYRTOS_ERR_PARAM;
    }
    else
    {
      /* �����ٽ��� */
      CRITICAL_ENTER ();

      /* ��������û����Ϣ������������ */
      if (qptr->num_msgs_stored == 0)
      {
        
        /* timeout>0 �������� */
        if (timeout >= 0)
        {

          /* ��ȡ��ǰ����TCB */
          curr_tcb_ptr = eCurrentContext();

          /* ��������Ƿ������������� */
          if (curr_tcb_ptr)
          {
            
            /* ����ǰ�������ӵ�receive���Ҷ����� */
            if (tcbEnqueuePriority (&qptr->getSuspQ, curr_tcb_ptr) == EASYRTOS_OK)
            {
              
              /* ������״̬����Ϊ���� */
              curr_tcb_ptr->state = TASK_PENDED;

              status = EASYRTOS_OK;

              /* ע�ᶨʱ���ص� */
              if (timeout)
              {
                /* ��䶨ʱ����Ҫ������ */
                timerData.tcb_ptr = curr_tcb_ptr;
                timerData.queue_ptr = qptr;
                timerData.suspQ = &qptr->getSuspQ;

                /* ���ص���Ҫ������ */
                timerCb.cb_func = eQueueTimerCallback;
                timerCb.cb_data = (POINTER)&timerData;
                timerCb.cb_ticks = timeout;

                /* ������TCB�д洢��ʱ���ص�������������ȡ������ */
                curr_tcb_ptr->pended_timo_cb = &timerCb;

                /* ע�ᶨʱ�� */
                if (eTimerRegister (&timerCb) != EASYRTOS_OK)
                {
                  /* ע��ʧ�� */
                  status = EASYRTOS_ERR_TIMER;

                  (void)tcb_dequeue_entry (&qptr->getSuspQ, curr_tcb_ptr);
                  curr_tcb_ptr->state = TASK_RUN;
                  curr_tcb_ptr->pended_timo_cb = NULL;
                }
              }

              /* ����Ҫע�ᶨʱ�� */
              else
              {
                curr_tcb_ptr->pended_timo_cb = NULL;
              }

              /* �˳��ٽ��� */
              CRITICAL_EXIT();
              
              if (status == EASYRTOS_OK)
              {
                /* ��ǰ�������ң����ǽ����õ����� */
                easyRTOSSched (FALSE);
                
                /* �´����񽫴Ӵ˴���ʼ���У���ʱ���б�ɾ�� ����timeout���� ���ߵ�����eQueueGive */
                status = curr_tcb_ptr->pendedWakeStatus;

                /** 
                 * ���pendedWakeStatus������ֵΪEASYRTOS_OK����˵��
                 * ��ȡ�ǳɹ��ģ���Ϊ������ֵ����˵���п��ܶ��б�ɾ��
                 * ����timeout���ڣ���ʱ����ֻ��Ҫ�˳��ͺ���
                 */
                if (status == EASYRTOS_OK)
                {
                  /* �����ٽ��� */
                  CRITICAL_ENTER();

                  /* ����Ϣ���Ƴ��� */
                  status = queue_remove (qptr, msgptr);

                  /* �˳��ٽ��� */
                  CRITICAL_EXIT();
                }
              }
            }
            else
            {
              /* ��������������б�ʧ�� */
              CRITICAL_EXIT ();
              status = EASYRTOS_ERR_QUEUE;
            }
          }
          else
          {
            /* �����������������ǣ��޷��������� */
            CRITICAL_EXIT ();
            status = EASYRTOS_ERR_CONTEXT;
          }
        }
        else
        {
          /* timeout == -1, ����Ҫ���������Ҷ��д�ʱ������Ϊ0 */
          CRITICAL_EXIT();
          status = EASYRTOS_WOULDBLOCK;
        }
      }
      else
      {
        /* ����Ҫ��������ֱ�Ӱ���Ϣ���Ƴ��� */
        status = queue_remove (qptr, msgptr);

        /* �˳��ٽ��� */
        CRITICAL_EXIT ();

        /**
         * ֻ�����������Ļ������õ�������
         * �жϻ�������eIntExit()���õ�������.
         */
        if (eCurrentContext())
          easyRTOSSched (FALSE);
      }
    }

    return (status);
}

/**
 * ����: ȡ�������е�����,���Զ���Ϊ��,����timeout�Ĳ�ֵͬ�в�ͬ�Ĵ���ʽ.
 * 1.timeout>0 ���ҵ��õ�����,��timeout���ڵ�ʱ�������񲢷���timeout��־
 * 2.timeout=0 �������ҵ��õ�����,ֱ���Ӷ����еȵ�����.
 * 3.timeout=-1 ����������,������Ϊ�ջ᷵�ض���Ϊ�ձ�־.
 * �����������ҵ�ʱ��,������õ�����.
 *
 * ����:
 * ����:                                        ���:
 * EASYRTOS_QUEUE *qptr ����ָ��                EASYRTOS_QUEUE *qptr ����ָ��
 * int32_t timeout timeoutʱ��,����������ʱ��   void *msgptr ������е���Ϣ
 * void *msgptr ������е���Ϣ                
 * 
 * ����:
 * EASYRTOS_OK �ɹ�
 * EASYRTOS_WOULDBLOCK �����ᱻ���ҵ�����timeoutΪ-1���Է�����
 * EASYRTOS_TIMEOUT �ź���timeout����
 * EASYRTOS_ERR_DELETED ��������������ʱ��ɾ��
 * EASYRTOS_ERR_CONTEXT ����������ĵ���
 * EASYRTOS_ERR_PARAM ��������
 * EASYRTOS_ERR_QUEUE ������������ж���ʧ��
 * EASYRTOS_ERR_TIMER ע�ᶨʱ��δ�ɹ�
 * 
 * ���õĺ���:
 * eCurrentContext();
 * tcbEnqueuePriority (&qptr->getSuspQ, curr_tcb_ptr);
 * eTimerRegister (&timerCb);
 * tcb_dequeue_entry (&qptr->getSuspQ, curr_tcb_ptr);
 * easyRTOSSched (FALSE);
 * queue_remove (qptr, msgptr);
 */
ERESULT eQueueGive (EASYRTOS_QUEUE *qptr, int32_t timeout, void *msgptr)
{
    CRITICAL_STORE;
    ERESULT status;
    QUEUE_TIMER timerData;
    EASYRTOS_TIMER timerCb;
    EASYRTOS_TCB *curr_tcb_ptr;

    /* ������� */
    if ((qptr == NULL) || (msgptr == NULL))
    {
        status = EASYRTOS_ERR_PARAM;
    }
    else
    {
        /* �����ٽ��� */
        CRITICAL_ENTER ();

        /* ���������������ҵ��ô˺��������� */
        if (qptr->num_msgs_stored == qptr->max_num_msgs)
        {
            /* timeout >= 0, ���񽫱����� */
            if (timeout >= 0)
            {

                /* ��ȡ��ǰ����TCB */
                curr_tcb_ptr = eCurrentContext();

                /* ����Ƿ������������� */
                if (curr_tcb_ptr)
                {
                    /* ����ǰ������ӵ�send�����б��� */
                    if (tcbEnqueuePriority (&qptr->putSuspQ, curr_tcb_ptr) == EASYRTOS_OK)
                    {
                        /* ��������״̬��־λΪ���� */
                        curr_tcb_ptr->state = TASK_PENDED;

                        status = EASYRTOS_OK;

                        /* timeout>0 ע�ᶨʱ���ص� */
                        if (timeout)
                        {
                            /* ��䶨ʱ����Ҫ������ */
                            timerData.tcb_ptr = curr_tcb_ptr;
                            timerData.queue_ptr = qptr;
                            timerData.suspQ = &qptr->putSuspQ;


                            /* ���ص���Ҫ������ */
                            timerCb.cb_func = eQueueTimerCallback;
                            timerCb.cb_data = (POINTER)&timerData;
                            timerCb.cb_ticks = timeout;

                            /* ������TCB�д洢��ʱ���ص�������������ȡ������ */
                            curr_tcb_ptr->pended_timo_cb = &timerCb;

                            /* ע�ᶨʱ�� */
                            if (eTimerRegister (&timerCb) != EASYRTOS_OK)
                            {
                                /* ע��ʧ�� */
                                status = EASYRTOS_ERR_TIMER;
                                
                                (void)tcb_dequeue_entry (&qptr->putSuspQ, curr_tcb_ptr);
                                curr_tcb_ptr->state = TASK_RUN;
                                curr_tcb_ptr->pended_timo_cb = NULL;
                            }
                        }

                        /* ����Ҫע�ᶨʱ�� */
                        else
                        {
                            curr_tcb_ptr->pended_timo_cb = NULL;
                        }

                        /* �˳��ٽ��� */
                        CRITICAL_EXIT ();

                        /* ����Ƿ�ע��ɹ� */
                        if (status == EASYRTOS_OK)
                        {
                            /* ��ǰ�������ң����ǽ����õ����� */
                            easyRTOSSched (FALSE);
                            
                            /* �´����񽫴Ӵ˴���ʼ���У���ʱ���б�ɾ�� ����timeout���� ���ߵ�����eQueueGive */
                            status = curr_tcb_ptr->pendedWakeStatus;

                            /** 
                             * ���pendedWakeStatus������ֵΪEASYRTOS_OK����˵��
                             * ��ȡ�ǳɹ��ģ���Ϊ������ֵ����˵���п��ܶ��б�ɾ��
                             * ����timeout���ڣ���ʱ����ֻ��Ҫ�˳��ͺ���
                             */
                            if (status == EASYRTOS_OK)
                            {
                                /* �����ٽ��� */
                                CRITICAL_ENTER();

                                /* ����Ϣ������� */
                                status = queue_insert (qptr, msgptr);

                                /* �˳��ٽ��� */
                                CRITICAL_EXIT();
                            }
                        }
                    }
                    else
                    {
                        /* ������������Ҷ���ʧ�� */
                        CRITICAL_EXIT();
                        status = EASYRTOS_ERR_QUEUE;
                    }
                }
                else
                {
                    /* �������������ģ������������� */
                    CRITICAL_EXIT ();
                    status = EASYRTOS_ERR_CONTEXT;
                }
            }
            else
            {
                /* timeout == -1, ����Ҫ���������Ҷ��д�ʱ������Ϊ0 */
                CRITICAL_EXIT();
                status = EASYRTOS_WOULDBLOCK;
            }
        }
        else
        {
            /* ������������ֱ�ӽ����ݸ��ƽ����� */
            status = queue_insert (qptr, msgptr);

            /* �˳��ٽ��� */
            CRITICAL_EXIT ();

            /**
             * ֻ�����������Ļ������õ�������
             * �жϻ�������eIntExit()���õ�������.
             */
            if (eCurrentContext())
                easyRTOSSched (FALSE);
        }
    }

    return (status);
}

/**
 * ����: ����ע��Ķ�ʱ���Ļص�����,�����ڵ����񷵻�EASYRTOS_TIMEOUT�ı�־.
 * �����ڵ������Ƴ����������б�,������Ready�б�.
 *
 * ����:
 * ����:                                                ���:
 * POINTER cb_data �ص��������ݰ�����Ҫ���ѵ�TCB����Ϣ   POINTER cb_data �ص��������ݰ�����Ҫ���ѵ�TCB����Ϣ                              
 * 
 * ����:void
 * 
 * ���õĺ���:
 * (void)tcb_dequeue_entry (timer_data_ptr->suspQ, timer_data_ptr->tcb_ptr);
 * (void)tcbEnqueuePriority (&tcb_readyQ, timer_data_ptr->tcb_ptr);
 */
static void eQueueTimerCallback (POINTER cb_data)
{
    QUEUE_TIMER *timer_data_ptr;
    CRITICAL_STORE;

    /* ��ȡ���ж�ʱ���ص�ָ�� */
    timer_data_ptr = (QUEUE_TIMER *)cb_data;

    /* �������Ƿ�Ϊ�� */
    if (timer_data_ptr)
    {
        /* �����ٽ���  */
        CRITICAL_ENTER ();

        /* ���������ñ�־λ����ʶ��������Ϊ��ʱ���ص����ѵ� */
        timer_data_ptr->tcb_ptr->pendedWakeStatus = EASYRTOS_TIMEOUT;

        /* ȡ����ʱ��ע�� */
        timer_data_ptr->tcb_ptr->pended_timo_cb = NULL;

        /* �������Ƴ������б��������Ƴ�receive����send�б� */
        (void)tcb_dequeue_entry (timer_data_ptr->suspQ, timer_data_ptr->tcb_ptr);

        /* ���������Ready���� */
        if (tcbEnqueuePriority (&tcb_readyQ, timer_data_ptr->tcb_ptr) == EASYRTOS_OK)
        {
          timer_data_ptr->tcb_ptr->state= TASK_READY;
        }

        /* �˳��ٽ��� */
        CRITICAL_EXIT ();
    }
}

/**
 * ����: ȡ�������е���Ϣ,�����������ڵȴ�����(ȡ����Ϣ��,���н���һ����λ),������
 * ��ȡ����ע��Ķ�ʱ��.
 *
 * ����:
 * ����:                                ���:
 * EASYRTOS_QUEUE *qptr ����ָ��        EASYRTOS_QUEUE *qptr ����ָ��                              
 * void* msgptr ȡ������Ϣ              void* msgptr ȡ������Ϣ
 *
 * ����:
 * EASYRTOS_OK �ɹ�
 * EASYRTOS_ERR_PARAM ��������
 * EASYRTOS_ERR_QUEUE ������������ж���ʧ��
 * EASYRTOS_ERR_TIMER ȡ����ʱ��ʧ��
 *
 * ���õĺ���:
 * memcpy ((uint8_t*)msgptr, ((uint8_t*)qptr->buff_ptr + qptr->remove_index), qptr->unit_size);
 * tcb_dequeue_head (&qptr->putSuspQ);
 * tcbEnqueuePriority (&tcb_readyQ, tcb_ptr);
 * eTimerCancel (tcb_ptr->pended_timo_cb);
 */
static ERESULT queue_remove (EASYRTOS_QUEUE *qptr, void* msgptr)
{
  ERESULT status;
  EASYRTOS_TCB *tcb_ptr;

  /* ������� */
  if ((qptr == NULL)) //|| (msgptr == NULL))
  {
    status = EASYRTOS_ERR_PARAM;
  }
  else
  {
    /* �����������ݣ����临�Ƴ��� */
    memcpy ((uint8_t*)msgptr, ((uint8_t*)qptr->buff_ptr + qptr->remove_index), qptr->unit_size);
    qptr->remove_index += qptr->unit_size;
    qptr->num_msgs_stored--;
    
    /* ����Ϊѭ���洢���ݣ�Ŀ����Ϊ�˼ӿ�����ٶ� */
    /* ����Ƿ�����remove_index */
    if (qptr->remove_index >= (qptr->unit_size * qptr->max_num_msgs))
        qptr->remove_index = 0;

    /* �����������ڵȴ����ͣ����份�� */    
    tcb_ptr = tcb_dequeue_head (&qptr->putSuspQ);
    if (tcb_ptr)
    {
      /* �����ҵ��������Ready�б� */
      if (tcbEnqueuePriority (&tcb_readyQ, tcb_ptr) == EASYRTOS_OK)
      {
        
        tcb_ptr->pendedWakeStatus = EASYRTOS_OK;
        tcb_ptr->state = TASK_READY;
        
        /* ��ע���˶�ʱ���ص�������ȡ�� */
        if ((tcb_ptr->pended_timo_cb != NULL)
            && (eTimerCancel (tcb_ptr->pended_timo_cb) != EASYRTOS_OK))
        {
          status = EASYRTOS_ERR_TIMER;
        }
        else
        {
          tcb_ptr->pended_timo_cb = NULL;

          /* �ɹ� */
          status = EASYRTOS_OK;
        }
      }
      else
      {
        /* ���������Ready�б�ʧ�� */              
        status = EASYRTOS_ERR_QUEUE;
      }
    }
    else
    {
      /* û������ȴ����� */
      status = EASYRTOS_OK;
    }
  }

  return (status);
}

/**
 * ����: ������м�����Ϣ,��������ȴ�������Ϣ(������Ϣ����ɻ�ȡ��Ϣ),������
 * ��ȡ����ע��Ķ�ʱ��.
 *
 * ����:
 * ����:                                ���:
 * EASYRTOS_QUEUE *qptr ����ָ��        EASYRTOS_QUEUE *qptr ����ָ��                              
 * void* msgptr �������Ϣ             void* msgptr �������Ϣ
 *
 * ����:
 * EASYRTOS_OK �ɹ�
 * EASYRTOS_ERR_PARAM ��������
 * EASYRTOS_ERR_QUEUE ������������ж���ʧ��
 * EASYRTOS_ERR_TIMER ȡ����ʱ��ʧ��
 *
 * ���õĺ���:
 * memcpy (((uint8_t*)qptr->buff_ptr + qptr->insert_index), (uint8_t*)msgptr, qptr->unit_size);
 * tcb_dequeue_head (&qptr->getSuspQ)
 * tcbEnqueuePriority (&tcb_readyQ, tcb_ptr);
 * eTimerCancel (tcb_ptr->pended_timo_cb);
 */
static ERESULT queue_insert (EASYRTOS_QUEUE *qptr, void *msgptr)
{
    ERESULT status;
    EASYRTOS_TCB *tcb_ptr;

    /* ������� */
    if ((qptr == NULL)|| (msgptr == NULL))
    {
        status = EASYRTOS_ERR_PARAM;
    }
    else
    {
        /* �������п���λ�ã������ݸ��ƽ�ȥ */
        memcpy (((uint8_t*)qptr->buff_ptr + qptr->insert_index), (uint8_t*)msgptr, qptr->unit_size);
        qptr->insert_index += qptr->unit_size;
        qptr->num_msgs_stored++;

        /* ����Ϊѭ���洢���ݣ�Ŀ����Ϊ�˼ӿ�����ٶ� */
        /* ����Ƿ�����remove_index */
        if (qptr->insert_index >= (qptr->unit_size * qptr->max_num_msgs))
            qptr->insert_index = 0;

        /* �����������ڵȴ����գ����份�� */    
        tcb_ptr = tcb_dequeue_head (&qptr->getSuspQ);
        if (tcb_ptr)
        {
            /* �����ҵ��������Ready�б� */
            if (tcbEnqueuePriority (&tcb_readyQ, tcb_ptr) == EASYRTOS_OK)
            {

                tcb_ptr->pendedWakeStatus = EASYRTOS_OK;
                tcb_ptr->state = TASK_READY;
                
                /* ��ע���˶�ʱ���ص�������ȡ�� */
                if ((tcb_ptr->pended_timo_cb != NULL)
                    && (eTimerCancel (tcb_ptr->pended_timo_cb) != EASYRTOS_OK))
                {
                    status = EASYRTOS_ERR_TIMER;
                }
                else
                {
                  
                    tcb_ptr->pended_timo_cb = NULL;

                    /* �ɹ� */
                    status = EASYRTOS_OK;
                }
            }
            else
            {
                /* ���������Ready�б�ʧ�� */  
                status = EASYRTOS_ERR_QUEUE;
            }
        }
        else
        {
            /* û������ȴ����� */
            status = EASYRTOS_OK;
        }
    }

    return (status);
}
