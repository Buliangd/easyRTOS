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
#include "easyRTOSSem.h"

/* ȫ�ֺ��� */
EASYRTOS_SEM eSemCreateCount (uint8_t initial_count);
EASYRTOS_SEM eSemCreateBinary (void);
EASYRTOS_SEM eSemCreateMutex (void);
ERESULT eSemDelete (EASYRTOS_SEM *sem);
ERESULT eSemTake (EASYRTOS_SEM *sem, int32_t timeout);
ERESULT eSemGive (EASYRTOS_SEM * sem);
ERESULT eSemResetCount (EASYRTOS_SEM *sem, uint8_t count);

/* ˽�к��� */
static void eSemTimerCallback (POINTER cb_data);

/**
 * ����: �����ź�������,��ʼ�������ź����ṹ���ڵĲ���,������.
 *
 * ����:
 * ����:                                   ���:
 * uint8_t initial_count ��ʼ��������      ��
 *
 * ����:
 * EASYRTOS_SEM
 * 
 * ���õĺ���:
 * ��.
 */
EASYRTOS_SEM eSemCreateCount (uint8_t initial_count)
{
  EASYRTOS_SEM sem;

  /* ���ó�ʼ���� */
  sem.count = initial_count;

  /* ��ʼ�����ź������ҵ�����Ķ��� */
  sem.suspQ = NULL;
  
  /* ��ʼ�����ź������� */
  sem.type = SEM_COUNTY;

  return sem;
}

/**
 * ����: ��ֵ�ź�������,��ʼ����ֵ�ź����ṹ���ڵĲ���,������.
 *
 * ����:
 * ����:                     ���:
 * ��                        ��
 *
 * ����:
 * EASYRTOS_SEM
 * 
 * ���õĺ���:
 * ��.
 */
EASYRTOS_SEM eSemCreateBinary (void)
{
  EASYRTOS_SEM sem;

  /* ���ó�ʼ���� */
  sem.count = 0;

  /* ��ʼ�����ź������ҵ�����Ķ��� */
  sem.suspQ = NULL;
  
  /* ��ʼ�����ź������� */
  sem.type = SEM_BINARY;

  return sem;
}

/**
 * ����: ����������,��ʼ���������ṹ���ڵĲ���,������.
 *
 * ����:
 * ����:                   ���:
 * ��                      ��
 *
 * ����:
 * EASYRTOS_SEM
 * 
 * ���õĺ���:
 * ��.
 */
EASYRTOS_SEM eSemCreateMutex (void)
{
    EASYRTOS_SEM sem;

    /* ��ʼ��ʱû��owner */
    sem.owner = NULL;

    /* ��ʼ������ */
    sem.count = 1;

    /* ��ʼ���������ҵ����� */
    sem.suspQ = NULL;
    
    /* ��ʼ�����ź������� */
    sem.type = SEM_MUTEX;
    return (sem);
}

/**
 * ����: ɾ���ź���,���������б����ź������ҵ��������Ready�б���.ͬʱȡ��������
 * ע��Ķ�ʱ��.�������񱻻���,�������������.
 *
 * ����:
 * ����:                                   ���:
 * EASYRTOS_SEM *sem �ź���ָ��            EASYRTOS_SEM *sem �ź���ָ��
 *
 * ����:
 * ���� EASYRTOS_OK �ɹ�
 * ���� EASYRTOS_ERR_QUEUE ��������õ�Ready������ʧ��
 * ���� EASYRTOS_ERR_TIMER ȡ����ʱ��ʧ��
 * ���� EASYRTOS_ERR_PARAM �����������
 * ���� EASYRTOS_ERR_DELETED �ź�������������ʱ��ɾ��
 * 
 * ���õĺ���:
 * tcb_dequeue_head (&sem->suspQ);
 * tcbEnqueuePriority (&tcb_readyQ, tcb_ptr);
 * eTimerCancel (tcb_ptr->pended_timo_cb);
 */
ERESULT eSemDelete (EASYRTOS_SEM *sem)
{
  ERESULT status;
  CRITICAL_STORE;
  EASYRTOS_TCB *tcb_ptr;
  uint8_t woken_threads = FALSE;

  /* ������� */
  if (sem == NULL)
  {
    status = EASYRTOS_ERR_PARAM;
  }
  else
  {
    status = EASYRTOS_OK;

    /* �������б����ҵ����� */
    while (1)
    {
      /* �����ٽ��� */
      CRITICAL_ENTER ();

      /* ����Ƿ����������� �����кܶ����񱻸��ź������� &sem->suspQΪ�������� */
      tcb_ptr = tcb_dequeue_head (&sem->suspQ);

      /* ���������ź������� */
      if (tcb_ptr)
      {
        /* �Ա����ҵ����񷵻ش����־ */
        tcb_ptr->pendedWakeStatus = EASYRTOS_ERR_DELETED;

        /* ������TCB����Ready������ */
        if (tcbEnqueuePriority (&tcb_readyQ, tcb_ptr) != EASYRTOS_OK)
        {
          /* ������ʧ�ܣ��˳��ٽ��� */
          CRITICAL_EXIT ();

          /* �˳�ѭ�������ؼ���Ready����ʧ�� */
          status = EASYRTOS_ERR_QUEUE;
          break;
        }
        
        /* �ɹ������������ΪREADY״̬ */
        else tcb_ptr->state = TASK_READY;

        /* ��������timeout��ȡ����Ӧ�Ķ�ʱ�� */
        if (tcb_ptr->pended_timo_cb)
        {
          if (eTimerCancel (tcb_ptr->pended_timo_cb) != EASYRTOS_OK)
          {
            /* ȡ����ʱ��ʧ�ܣ��˳��ٽ��� */
            CRITICAL_EXIT ();

            /* �˳�ѭ�������ش����ʶ */
            status = EASYRTOS_ERR_TIMER;
            break;
          }

          /* ��־û��timeout��ʱ��ע�� */
          tcb_ptr->pended_timo_cb = NULL;
        }

        /* �˳��ٽ��� */
        CRITICAL_EXIT ();

        /* ��������� */
        woken_threads = TRUE;
      }

      /* û�б����ҵ������� */
      else
      {
        /* �˳��ٽ���������ѭ�� */
        CRITICAL_EXIT ();
        break;
      }
    }

    /* �����񱻻�������õ����� */
    if (woken_threads == TRUE)
    {
      /**
       *  ֻ���������в����е����������ж���ʱ�����˳��ж�
       *  ʱeIntExit()����
       */
      if (eCurrentContext())
          easyRTOSSched (FALSE);
    }
  }

  return (status);
}

/**
 * ����: ��ȡ�ź���,���ź�������Ϊ0,����timeout�Ĳ�ֵͬ���ź����Ĳ�ͬ�����в�ͬ�Ĵ���ʽ.
 *
 * һ����ֵ�ź����ͼ����ź���
 * 1.timeout>0 ���ҵ��õ�����,��timeout���ڵ�ʱ�������񲢷���timeout��־
 * 2.timeout=0 �������ҵ��õ�����,ֱ����ȡ���ź���.
 * 3.timeout=-1 ����������,���ź�������Ϊ0�᷵���ź���Ϊ0�ı�־.
 *
 * ����������
 * ��������Ϊӵ���ߣ������ݹ����ģʽ��������Ϊ��ֵ����������������
 * �������߲���ӵ���ߣ������timeout�Ĳ�ֵͬ�����µĴ���ʽ��
 * 1.timeout>0 ���ҵ��õ�����,��timeout���ڵ�ʱ�������񲢷���timeout��־
 * 2.timeout=0 �������ҵ��õ�����,ֱ����ȡ���ź���.
 * 3.timeout=-1 ����������,���ź�������Ϊ0�᷵���ź���Ϊ0�ı�־.
 *
 * �����������ҵ�ʱ��,������õ�����.
 *
 * ����:
 * ����:                                        ���:
 * EASYRTOS_SEM *sem  �ź���ָ��                EASYRTOS_SEM *sem  �ź���ָ��
 * int32_t timeout timeoutʱ��,����������ʱ��                  
 * 
 * ����:
 * EASYRTOS_OK �ɹ�
 * EASYRTOS_TIMEOUT �ź���timeout����
 * EASYRTOS_WOULDBLOCK ����Ϊ0��ʱ��timeout=-1
 * EASYRTOS_ERR_DELETED �ź�������������ʱ��ɾ��
 * EASYRTOS_ERR_CONTEXT ����������ĵ���
 * EASYRTOS_ERR_PARAM  ����Ĳ���
 * EASYRTOS_ERR_QUEUE ������������ж���ʧ��
 * EASYRTOS_ERR_TIMER ע��û�гɹ�
 * EASYRTOS_SEM_UINIT �ź���û�б���ʼ��
 * 
 * ���õĺ���:
 * eCurrentContext();
 * tcbEnqueuePriority (&sem->suspQ, curr_tcb_ptr);
 * eTimerRegister (&timerCb);
 * (void)tcb_dequeue_entry (&sem->suspQ, curr_tcb_ptr);
 * easyRTOSSched (FALSE);
 */
ERESULT eSemTake (EASYRTOS_SEM *sem, int32_t timeout)
{
  CRITICAL_STORE;
  ERESULT status;
  SEM_TIMER timerData;
  EASYRTOS_TIMER timerCb;
  EASYRTOS_TCB *curr_tcb_ptr;

  /* ������� */
  if (sem == NULL)
  {
    status = EASYRTOS_ERR_PARAM;
  }
  else if (sem->type == NULL)
  {
    status = EASYRTOS_SEM_UINIT;
  }
  else
  {
    /* �����ٽ��� ��ֹ�ڴ�ʱ�ź��������仯 */
    CRITICAL_ENTER ();
    
    /* ��ȡ�������������TCB */
    curr_tcb_ptr = eCurrentContext();
        
    /**
     * ����Ƿ�������������(�������ж�),��ΪMUTEX��Ҫһ��ӵ����,���Բ���
     * ��ISR����
     */
    if (curr_tcb_ptr == NULL)
    {
        /* �˳��ٽ��� */
        CRITICAL_EXIT ();

        /* ����������������,�޷��������� */
        status = EASYRTOS_ERR_CONTEXT;
    }

    /** 
     * ��Ϊ��ֵ�źŻ��߼����ź�����ʱ��,���ж�count�Ƿ�Ϊ0.
     * ��Ϊ�������ź���,���ж��Ƿ���������ӵ��������ͬ.
     * ������һ,�����Ҹ�����. 
     */
    else if (((sem->type != SEM_MUTEX) && (sem->count == 0)) || \\
             ((sem->type == SEM_MUTEX) && (sem->owner != curr_tcb_ptr) && (sem->owner != NULL)))
    {
      /* ��timeout >= 0 ���������� */
      if (timeout >= 0)
      {
        /* CountΪ0, �������� */

        /* ������������������ */
        if (curr_tcb_ptr)
        {
          /* �������������ź����������б� */
          if (tcbEnqueuePriority (&sem->suspQ, curr_tcb_ptr) != EASYRTOS_OK)
          {
            /* ��ʧ�ܣ��˳��ٽ��� */
            CRITICAL_EXIT ();

            /* ���ش��� */
            status = EASYRTOS_ERR_QUEUE;
          }
          else
          {
            /* ������״̬����Ϊ���� */
            curr_tcb_ptr->state = TASK_PENDED;
            
            status = EASYRTOS_OK;

            /* ����timeout��ֵ�������Ƿ���Ҫע�ᶨʱ���ص� */
            if (timeout)
            {
              /* ����ص���Ҫ������ */
              timerData.tcb_ptr = curr_tcb_ptr;
              timerData.sem_ptr = sem;

              /* ��ʱ���ص���Ҫ������ */
              timerCb.cb_func = eSemTimerCallback;
              timerCb.cb_data = (POINTER)&timerData;
              timerCb.cb_ticks = timeout;

              /**
               * ���涨ʱ���ص���TCB�У����ź���GIVE��ʱ�򣬷���ȡ��
               * timeoutע��Ķ�ʱ��
               */
              curr_tcb_ptr->pended_timo_cb = &timerCb;

              /* ע��timeout�Ķ�ʱ�� */
              if (eTimerRegister (&timerCb) != EASYRTOS_OK)
              {
                /* ��ע��ʧ�ܣ����ش��� */
                status = EASYRTOS_ERR_TIMER;

                /* ������� */
                (void)tcb_dequeue_entry (&sem->suspQ, curr_tcb_ptr);
                curr_tcb_ptr->state = TASK_READY;
                curr_tcb_ptr->pended_timo_cb = NULL;
              }
            }

            /* û��timeout���� */
            else
            {
              curr_tcb_ptr->pended_timo_cb = NULL;
            }

            /* �˳��ٽ��� */
            CRITICAL_EXIT ();

            /* ����Ƿ��д����� */
            if (status == EASYRTOS_OK)
            {
              /* �������ң����õ���������һ���µ����� */
              easyRTOSSched (FALSE);

              /**
               * ͨ��eSemGive()���ѻ᷵��EASYRTOS_OK����timeoutʱ�䵽����
               * EASYRTOS_TIMEOUT�����ź�����ɾ��ʱ������EASYRTOS_ERR_DELETED
               */
              status = curr_tcb_ptr->pendedWakeStatus;

              /**
               * ���߳���EASYRTOS_OK������±�����ʱ�����������������ź���
               * �������ѿ���Ȩ���������������ϣ�֮ǰ�����������ź���������
               * Ȼ�������������ź�������������Ϊ���ܹ����������ȼ����ߵ�
               * ������ռ�����ǰѼ��ټ����ĵط�������eSemGive()��
               */
            }
          }
        }
        else
        {
          /* �˳��ٽ��� */
          CRITICAL_EXIT ();

          /* �������������ģ��������� */
          status = EASYRTOS_ERR_CONTEXT;
        }
      }
      else
      {
        /* timeout == -1, ����Ҫ���� */
        CRITICAL_EXIT();
        status = EASYRTOS_WOULDBLOCK;
      }
    }
    else
    {
      switch (sem->type)
      {
        case SEM_BINARY:
        case SEM_COUNTY:
            sem->count--;
            status = EASYRTOS_OK;
          break;
        case SEM_MUTEX:
          
          /* Count����0������Count��ֵ�������� */
          if (sem->owner == NULL)
          {
            sem->owner = curr_tcb_ptr;
          }
      
          /* Count����0������Count��ֵ�������� */
          if (sem->count>-32768)
          {
            sem->count--;
        
            /* �ɹ� */
            status = EASYRTOS_OK;
          }
          else {
            status = EASYRTOS_ERR_OVF;
          }
          break;
        default:
          status = EASYRTOS_SEM_UINIT;
      }

      /* �˳��ٽ��� */
      CRITICAL_EXIT ();
  
    }
  }

  return (status);
}

/**
 * ����: �����ź���,�����ź������Ͳ�ͬ�������²�ͬ��Ӧ��
 * 1����ֵ�ź���
 * ������Ϊ0��ʱ�������1�������Ѿ�Ϊ1��᷵���������
 * 2�������ź���
 * ������1������������127ʱ����᷵���������
 * 3��������
 * ��������ӵ���ߵ��õ�ʱ��������<=0���������1���������ﵽ1ʱ�����ӵ������
 * ����ӵ���ߵ��õ�ʱ�򷵻�EASYRTOS_ERR_OWNERSHIP
 * �����������ҵ�ʱ��,������õ�����.
 *
 * ����:
 * ����:                                      ���:
 * EASYRTOS_SEM * sem �ź���ָ��              EASYRTOS_SEM * sem �ź���ָ��       
 * 
 * ����:
 * EASYRTOS_OK �ɹ�
 * EASYRTOS_ERR_OVF �����ź���count>32767(>32767)
 * EASYRTOS_ERR_PARAM ����Ĳ���
 * EASYRTOS_ERR_QUEUE ������������ж���ʧ��
 * EASYRTOS_ERR_TIMER ע�ᶨʱ��δ�ɹ�
 * EASYRTOS_ERR_BIN_OVF ��ֵ�ź���count�Ѿ�Ϊ1
 * EASYRTOS_SEM_UINIT �ź���û�б���ʼ��
 * EASYRTOS_ERR_OWNERSHIP ���Խ���Mutex��������Mutexӵ����
 * 
 * ���õĺ���:
 * eCurrentContext();
 * tcb_dequeue_head (&sem->suspQ);
 * tcbEnqueuePriority (&tcb_readyQ, tcb_ptr);
 * eTimerCancel (tcb_ptr->pended_timo_cb);
 * easyRTOSSched (FALSE);
 */
ERESULT eSemGive (EASYRTOS_SEM * sem)
{
  ERESULT status;
  CRITICAL_STORE;
  EASYRTOS_TCB *tcb_ptr;
  EASYRTOS_TCB *curr_tcb_ptr;
  
  /* ������� */
  if (sem == NULL)
  {
    status = EASYRTOS_ERR_PARAM;
  }
  else if (sem->type == NULL)
  {
    status = EASYRTOS_SEM_UINIT;
  }
  else
  {
    
    /* ��ȡ�������е������TCB */
    curr_tcb_ptr = eCurrentContext();
        
    /* �����ٽ��� */
    CRITICAL_ENTER ();
    
    if (sem->type == SEM_MUTEX && sem->owner != curr_tcb_ptr)
    {
        /* �˳��ٽ��� */
        CRITICAL_EXIT ();
        
        status = EASYRTOS_ERR_OWNERSHIP;
    }

    /* �����ź������ҵ���������Ready�����б� */
    else 
    {
      
      if (sem->suspQ && sem->count == 0)
      {
        sem->owner = NULL;
        //if ( sem->type == SEM_MUTEX )sem->count++;
        tcb_ptr = tcb_dequeue_head (&sem->suspQ);
        if (tcbEnqueuePriority (&tcb_readyQ, tcb_ptr) != EASYRTOS_OK)
        {
          
          /* ������Ready�б�ʧ�ܣ��˳��ٽ��� */
          CRITICAL_EXIT ();

          status = EASYRTOS_ERR_QUEUE;
        }
        else
        {
          
          /* ���ȴ������񷵻�EASYRTOS_OK */
          tcb_ptr->pendedWakeStatus = EASYRTOS_OK;
          tcb_ptr->state = TASK_READY;
          
          /* ��������Ϊ�µĻ�����ower */
          sem->owner = tcb_ptr;
          
          /* ������ź���timeoutע��Ķ�ʱ�� */
          if ((tcb_ptr->pended_timo_cb != NULL)
              && (eTimerCancel (tcb_ptr->pended_timo_cb) != EASYRTOS_OK))
          {
            
              /* �����ʱ��ʧ�� */
              status = EASYRTOS_ERR_TIMER;
          }
          else
          {
            
              /* û��timeout��ʱ��ע�� */
              tcb_ptr->pended_timo_cb = NULL;

              /* �ɹ� */
              status = EASYRTOS_OK;
          }

          /* �˳��ٽ��� */
          CRITICAL_EXIT ();

          if (eCurrentContext())
              easyRTOSSched (FALSE);
        }
      }
    

      /* ��û�����񱻸��ź������ң�������count��Ȼ�󷵻� */
      else
      {
        switch (sem->type)
        {
          case SEM_COUNTY:
            
            /* ����Ƿ���� */
            if (sem->count == 32767)
            {
              
              /* ���ش����ʶ */
              status = EASYRTOS_ERR_OVF;
            }
            else
            {
              
              /* ����count������ */
              sem->count++;
              status = EASYRTOS_OK;
            }
          break;
          
          case SEM_BINARY:
            
            /* ����Ƿ��Ѿ�Ϊ1 */
            if (sem->count == 1)
            {
              
              /* ���ش����ʶ */
              status = EASYRTOS_ERR_OVF;
            }
            else
            {
              
              /* ����count������ */
              sem->count = 1;
              status = EASYRTOS_OK;
            }
          break;
          
          case SEM_MUTEX:
            if (sem->count>1)
            {
              
              /* ���ش����ʶ */
              status = EASYRTOS_ERR_OVF;
            }
            else
            {
              sem->count++;
              //���� ��sem->count==1 �����ӵ����
              if (sem->count>=1)sem->owner=NULL;
              status = EASYRTOS_OK;
            }
          break;
        }
      }

      /* �˳��ٽ��� */
      CRITICAL_EXIT ();
    }
  }

  return (status);
}

/**
 * ����: ���ü����ź�����Count
 *
 * ����:
 * ����:                                        ���:
 * EASYRTOS_SEM *sem �ź���ָ��                 EASYRTOS_SEM *sem �ź���ָ��
 * uint8_t count���õ�Count��               
 * 
 * ����:
 * ���� EASYRTOS_OK �ɹ�
 * ���� EASYRTOS_ERR_PARAM ����Ĳ���
 * 
 * ���õĺ���:
 * ��
 */
ERESULT eSemResetCount (EASYRTOS_SEM *sem, uint8_t count)
{
  ERESULT status;
  CRITICAL_STORE;
  EASYRTOS_TCB *tcb_ptr;
  

  
  /* ������� */
  if (sem == NULL || sem->type != SEM_COUNTY)
  {
    status = EASYRTOS_ERR_PARAM;
  }
  else
  {
    if (sem->suspQ && sem->count == 0)
    {
      /* �����ٽ��� */
      CRITICAL_ENTER ();
      tcb_ptr = tcb_dequeue_head (&sem->suspQ);
      if (tcbEnqueuePriority (&tcb_readyQ, tcb_ptr) != EASYRTOS_OK)
      {
        
        /* ������Ready�б�ʧ�ܣ��˳��ٽ��� */
        CRITICAL_EXIT ();

        status = EASYRTOS_ERR_QUEUE;
      }
      else
        CRITICAL_EXIT ();
    }
    
    /* ����countֵ */
    sem->count = count;

    /* �ɹ� */
    status = EASYRTOS_OK;
  }
  return (status);  
}

/**
 * ����: �ź���ע��Ķ�ʱ���Ļص�����,�����ڵ����񷵻�EASYRTOS_TIMEOUT�ı�־.
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
static void eSemTimerCallback (POINTER cb_data)
{
    SEM_TIMER *timer_data_ptr;
    CRITICAL_STORE;

    /* ��ȡSEM_TIMER�ṹָ�� */
    timer_data_ptr = (SEM_TIMER *)cb_data;

    /* �������Ƿ�Ϊ�� */
    if (timer_data_ptr)
    {
      /* �����ٽ��� */
      CRITICAL_ENTER ();

      /* ���ñ�־����������������timeout���ڶ����ѵ�  */
      timer_data_ptr->tcb_ptr->pendedWakeStatus = EASYRTOS_TIMEOUT ;

      /* ���timeout��ʱ��ע�� */
      timer_data_ptr->tcb_ptr->pended_timo_cb = NULL;

      /* �������Ƴ��ź������Ҷ��� */
      (void)tcb_dequeue_entry (&timer_data_ptr->sem_ptr->suspQ, timer_data_ptr->tcb_ptr);

      /* ���������Ready���� */
      if (tcbEnqueuePriority (&tcb_readyQ, timer_data_ptr->tcb_ptr) == EASYRTOS_OK)
      {
        timer_data_ptr->tcb_ptr->state = TASK_READY;
      }
      /* �˳��ٽ��� */
      CRITICAL_EXIT ();

      /* ����û����������������Ϊ֮�����˳�timer ISR��ʱ���ͨ��atomIntExit()���� */
    }
}
