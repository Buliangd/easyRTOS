#include "easyRTOS.h"
#include "easyRTOSkernel.h"
#include "easyRTOSport.h"
#include "easyRTOSTimer.h"
/* �������� */

/* timer���е�ָ�� */
static EASYRTOS_TIMER *timer_queue = NULL;

/**
 *  easyRTOSϵͳ�δ�����
 */
static uint32_t systemTicks = 0;

/* ˽�к���  */
static void eTimerCallbacks (void);
static void eTimerDelayCallback (POINTER cb_data);

/* ȫ�ֺ���  */
void eTimerCallbacks (void);
void eTimerTick (void);
ERESULT eTimerRegister (EASYRTOS_TIMER *timer_ptr);
ERESULT eTimerCancel (EASYRTOS_TIMER *timer_ptr);
ERESULT eTimerDelay (uint32_t ticks);
uint32_t eTimeGet(void);
void eTimeSet(uint32_t newTime);

/**
 * ���� EASYRTOS_OK
 * ���� EASYRTOS_ERR_PARAM
 */
ERESULT eTimerRegister (EASYRTOS_TIMER *timer_ptr)
{
    ERESULT status;
    CRITICAL_STORE;

    /**
     *  ������
     */
    if ((timer_ptr == NULL) || (timer_ptr->cb_func == NULL)
        || (timer_ptr->cb_ticks == 0))
    {
        /** 
         *  ���󷵻�
         */
        status = EASYRTOS_ERR_PARAM;
    }
    else
    {
        /** 
         *  �����ٽ���
         */
        CRITICAL_ENTER ();

        /*
         *  timer�����б�
         *
         *  �б���û��˳���,���е�timer�����б��ͷ����.ÿ��ϵͳ����,���б�
         *  �е����м����������1,��count����0,��timer�Ļص������ᱻ����.
         */
        if (timer_queue == NULL)
        {
            /** 
             *  �б�Ϊ��,����ͷ�б��ͷ
             */
            timer_ptr->next_timer = NULL;
            timer_queue = timer_ptr;
        }
        else
        {
            /** \
             *  �б��Ѿ�����timer,���б�ͷ�����µ�timer
             */
            timer_ptr->next_timer = timer_queue;
            timer_queue = timer_ptr;
        }

        /**
         *  �˳��ٽ���
         */
        CRITICAL_EXIT ();

        /** 
         *  ע��ɹ� 
         */
        status = EASYRTOS_OK;
    }

    return (status);
}


/**
 * ���� EASYRTOS_OK
 * ���� EASYRTOS_ERR_PARAM
 * ���� EASYRTOS_ERR_NOT_FOUND
 */
ERESULT eTimerCancel (EASYRTOS_TIMER *timer_ptr)
{
    ERESULT status = EASYRTOS_ERR_NOT_FOUND;
    EASYRTOS_TIMER *prev_ptr, *next_ptr;
    CRITICAL_STORE;

    /** 
     *  ������
     *  Parameter check 
     */
    if (timer_ptr == NULL)
    {
        /** 
         *  �������󷵻�
         */
        status = EASYRTOS_ERR_PARAM;
    }
    else
    {
        /**
         *  �����ٽ���
         */
        CRITICAL_ENTER ();

        /** 
         *  ��Ѱ�б�,�ҵ���ص�timer
         */
        prev_ptr = next_ptr = timer_queue;
        while (next_ptr)
        {
            /** 
             *  �Ƿ�������Ѱ�ҵ�timer
             */
            if (next_ptr == timer_ptr)
            {
                if (next_ptr == timer_queue)
                {
                    /** 
                     *  �Ƴ������б�ͷ
                     */
                    timer_queue = next_ptr->next_timer;
                }
                else
                {
                    /** 
                     *  �Ƴ������б��л����б�β 
                     */
                    prev_ptr->next_timer = next_ptr->next_timer;
                }

                /** 
                 *  ɾ���ɹ� 
                 */
                status = EASYRTOS_OK;
                break;
            }

            prev_ptr = next_ptr;
            next_ptr = next_ptr->next_timer;

        }

        /** 
         *  �˳��ٽ���
         */
        CRITICAL_EXIT ();
     }
  return (status);
}

/**
 * @���� EASYRTOS_OK 
 * @���� EASYRTOS_ERR_PARAM 
 * @���� EASYRTOS_ERR_CONTEXT 
 */
ERESULT eTimerDelay (uint32_t ticks)
{
    EASYRTOS_TCB *curr_tcb_ptr;
    EASYRTOS_TIMER timerCb;
    DELAY_TIMER timerData;
    CRITICAL_STORE;
    ERESULT status;

    /** 
     *  ��ȡ�������е�����TCB 
     */
    curr_tcb_ptr = eCurrentContext();

    /** 
     *  ������ 
     */
    if (ticks == 0)
    {
        /** 
         *  ���󷵻�
         */
        status = EASYRTOS_ERR_PARAM;
    }

    /** 
     *  ����Ƿ�������������
     */
    else if (curr_tcb_ptr == NULL)
    {
        /** 
         *  û��������������
         */
        status = EASYRTOS_ERR_CONTEXT;
    }

    else
    {
        /**
         *  �����ٽ���,�����������
         */
        CRITICAL_ENTER ();

        /** 
         *  ������������״̬ΪDelay
         */
        curr_tcb_ptr->state = TASK_DELAY;

        /** 
         *  ע��timer�Ļص�����. 
         */

        /** 
         *  ����ص�����
         */
        timerData.tcb_ptr = curr_tcb_ptr;

        timerCb.cb_func = eTimerDelayCallback;
        timerCb.cb_data = (POINTER)&timerData;
        timerCb.cb_ticks = ticks;

        /**
         *  �洢��ʱ�ص�(��ʱ���ǲ���ʹ����)
         */
        curr_tcb_ptr->delay_timo_cb = &timerCb;

        /** 
         *  ע�ᶨʱ���ص�
         */
        if (eTimerRegister (&timerCb) != EASYRTOS_OK)
        {
            /** 
             *  �˳��ٽ���
             */
            CRITICAL_EXIT ();

            /** 
             *  timerע��δ�ɹ�
             */
            status = EASYRTOS_ERR_TIMER;
        }
        else
        {
            /**
             *  �˳��ٽ���
             */
            CRITICAL_EXIT ();

            /** 
             *  timerע��ɹ�
             */
            status = EASYRTOS_OK;

            /** 
             *  �������е������ӳ�,������������ʼ����
             */
            easyRTOSSched (FALSE);
        }
    }

    return (status);
}

void eTimerTick (void)
{
  /**
   *  ��ϵͳ������ʱ�������
   */
  if (easyRTOSStarted)
  {
    /**
     *  ����ϵͳtick����
     */
    
    systemTicks++;

    /** 
     *  ����Ƿ��к�����Ҫ�ص�. 
     */
    eTimerCallbacks ();
  }
}

static void eTimerCallbacks (void)
{
  EASYRTOS_TIMER *prev_ptr = NULL, *next_ptr = NULL, *saved_next_ptr = NULL;
  EASYRTOS_TIMER *callback_list_tail = NULL, *callback_list_head = NULL;

  /**
   *  ��������,�������ж�ʱ���ļ���,����Ƿ���Ҫ�ص�.
   */
  prev_ptr = next_ptr = timer_queue;
  while (next_ptr)
  {
    /** 
     *  ��next timer�������б��� 
     */
    saved_next_ptr = next_ptr->next_timer;

    /** 
     *  timer ����Ƿ���?
     */
    if (--(next_ptr->cb_ticks) == 0)
    {
      /** 
       *  ���б����Ƴ������
       */
      if (next_ptr == timer_queue)
      {
        /** 
         *  �Ƴ����б��ͷ
         */
        timer_queue = next_ptr->next_timer;
      }
      else
      {
        /** 
         *  �Ƴ����б��л����б�β
         */
        prev_ptr->next_timer = next_ptr->next_timer;
      }

      /*
       *  ������ڼ�����Ҫִ�еĻص������б�.�����Ǳ����������б�֮��
       *  ��ִ��,��Ϊ�п���֮���лص���Ҫִ��.

       *  �ظ�����EASYRTOS_TIMER�ṹ��� next_ptr ָ��������ص��б�.
       */
      if (callback_list_head == NULL)
      {
        /** 
         *  ���б�����ӵ�һ���ص�.     
         */
        callback_list_head = callback_list_tail = next_ptr;
      }
      else
      {
        /** 
         *  ���б�β��ӻص�   
         */
        callback_list_tail->next_timer = next_ptr;
        callback_list_tail = callback_list_tail->next_timer;
      }

      /**
       *  ��Ǹ�timer���ǻص��б�����һ��
       */
      next_ptr->next_timer = NULL;

    }

    /**
     *  �ص����û�е���.�ڴ��˳�,��������countֵ
     */
    else
    {
      prev_ptr = next_ptr;
    }

    next_ptr = saved_next_ptr;
  }

  /**
   *  ����Ƿ��лص�����.
   */
  if (callback_list_head)
  {
      /**
       *  ���������ص��б�.
       *  Walk the callback list 
       */
      next_ptr = callback_list_head;
      while (next_ptr)
      {
          /**
           *  �����б��е� next timer,�����б��޸�.  
           */
          saved_next_ptr = next_ptr->next_timer;

          /** 
           *  ���ûص�
           */
          if (next_ptr->cb_func)
          {
              next_ptr->cb_func (next_ptr->cb_data);
          }

          /**
           *  �ҵ��ص��б��е���һ���ص�.
           */
          next_ptr = saved_next_ptr;
      }
  }
}

static void eTimerDelayCallback (POINTER cb_data)
{
    DELAY_TIMER *timer_data_ptr;
    CRITICAL_STORE;

    /** 
     *  ��ȡDELAY_TIMER�Ľṹ��ָ��.
     */
    timer_data_ptr = (DELAY_TIMER *)cb_data;

    /** 
     *  �������Ƿ�ΪNULL
     */
    if (timer_data_ptr)
    {
        /** 
         *  �����ٽ���
         */
        CRITICAL_ENTER ();

        /** 
         *  ���������Ready����
         */
        (void)tcbEnqueuePriority (&tcb_readyQ, timer_data_ptr->tcb_ptr);

        /** 
         *  �˳��ٽ���
         */
        CRITICAL_EXIT ();
    }
}

uint32_t eTimeGet(void)
{
    return (systemTicks);
}

void eTimeSet(uint32_t newTime)
{
    systemTicks = newTime;
}
