#include "easyRTOS.h"
#include "easyRTOSkernel.h"
#include "easyRTOSport.h"
#include "easyRTOSTimer.h"
/* Local data */

/** Pointer to the head of the outstanding timers queue */
static EASYRTOS_TIMER *timer_queue = NULL;

/**
 *  number of easyRTOS system ticks
 *  easyRTOSϵͳ�δ�����
 */
static uint32_t systemTicks = 0;

/* private function  */
static void eTimerCallbacks (void);
static void eTimerDelayCallback (POINTER cb_data);

/* public function  */
void eTimerCallbacks (void);
void eTimerTick (void);
ERESULT eTimerRegister (EASYRTOS_TIMER *timer_ptr);
ERESULT eTimerCancel (EASYRTOS_TIMER *timer_ptr);
ERESULT eTimerDelay (uint32_t ticks);
uint32_t eTimeGet(void);
void eTimeSet(uint32_t newTime);

/**
 * return EASYRTOS_OK
 * return EASYRTOS_ERR_PARAM
 */
ERESULT eTimerRegister (EASYRTOS_TIMER *timer_ptr)
{
    ERESULT status;
    CRITICAL_STORE;

    /**
     *  ������
     *  Parameter check   
     */
    if ((timer_ptr == NULL) || (timer_ptr->cb_func == NULL)
        || (timer_ptr->cb_ticks == 0))
    {
        /** 
         *  ���󷵻�
         *  Return error 
         */
        status = EASYRTOS_ERR_PARAM;
    }
    else
    {
        /** 
         *  �����ٽ���
         *  Protect the list 
         */
        CRITICAL_ENTER ();

        /*
         *  timer�����б�
         *
         *  �б���û��˳���,���е�timer�����б��ͷ����.ÿ��ϵͳ����,���б�
         *  �е����м����������1,��count����0,��timer�Ļص������ᱻ����.
         *
         *  Enqueue in the list of timers.
         *  
         *  The list is not ordered, all timers are inserted at the start
         *  of the list. On each system tick increment the list is walked
         *  and the remaining ticks count for that timer is decremented.
         *  Once the remaining ticks reaches zero, the timer callback is
         *  made.
         *
         */
        if (timer_queue == NULL)
        {
            /** 
             *  List is empty, insert new head 
             *  �б�Ϊ��,����ͷ�б��ͷ
             */
            timer_ptr->next_timer = NULL;
            timer_queue = timer_ptr;
        }
        else
        {
            /** \
             *  �б��Ѿ�����timer,���б�ͷ�����µ�timer
             *  List has at least one entry, enqueue new timer before 
             */
            timer_ptr->next_timer = timer_queue;
            timer_queue = timer_ptr;
        }

        /*
         *  �˳��ٽ���
         *  End of list protection 
         */
        CRITICAL_EXIT ();

        /** 
         *  ע��ɹ� 
         *  Successful 
         */
        status = EASYRTOS_OK;
    }

    return (status);
}


/**
 * return EASYRTOS_OK
 * return EASYRTOS_ERR_PARAM
 * return EASYRTOS_ERR_NOT_FOUND
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
         *  Return error 
         */
        status = EASYRTOS_ERR_PARAM;
    }
    else
    {
        /**
         *  �����ٽ���
         *  Protect the list 
         */
        CRITICAL_ENTER ();

        /** 
         *  ��Ѱ�б�,�ҵ���ص�timer
         *  Walk the list to find the relevant timer 
         */
        prev_ptr = next_ptr = timer_queue;
        while (next_ptr)
        {
            /** 
             *  �Ƿ�������Ѱ�ҵ�timer
             *  Is this entry the one we're looking for? 
             */
            if (next_ptr == timer_ptr)
            {
                if (next_ptr == timer_queue)
                {
                    /** 
                     *  �Ƴ������б�ͷ
                     *  We're removing the list head  
                     */
                    timer_queue = next_ptr->next_timer;
                }
                else
                {
                    /** 
                     *  �Ƴ������б��л����б�β
                     *  We're removing a mid or tail TCB   
                     */
                    prev_ptr->next_timer = next_ptr->next_timer;
                }

                /** 
                 *  ɾ���ɹ�
                 *  Successful   
                 */
                status = EASYRTOS_OK;
                break;
            }

            prev_ptr = next_ptr;
            next_ptr = next_ptr->next_timer;

        }

        /** 
         *  �˳��ٽ���
         *  End of list protection 
         */
        CRITICAL_EXIT ();
     }
  return (status);
}

/**
 * @return EASYRTOS_OK Successful delay
 * @return EASYRTOS_ERR_PARAM Bad parameter (ticks must be non-zero)
 * @return EASYRTOS_ERR_CONTEXT Not called from task context
 */
ERESULT eTimerDelay (uint32_t ticks)
{
    EASYRTOS_TCB *curr_tcb_ptr;
    EASYRTOS_TIMER timerCb;
    DELAY_TIMER timerData;
    CRITICAL_STORE;
    ERESULT status;

    /** 
     *  ��ȡ�������е�TCB
     *  Get the current TCB  
     */
    curr_tcb_ptr = eCurrentContext();

    /** 
     *  ������
     *  Parameter check 
     */
    if (ticks == 0)
    {
        /** 
         *  ���󷵻�
         *  Return error 
         */
        status = EASYRTOS_ERR_PARAM;
    }

    /** 
     *  ����Ƿ�������������
     *  Check we are actually in task context 
     */
    else if (curr_tcb_ptr == NULL)
    {
        /** 
         *  û��������������
         *  Not currently in task context, can't delay 
         */
        status = EASYRTOS_ERR_CONTEXT;
    }

    else
    {
        /**
         *  �����ٽ���,�����������
         *  Protect the system queues 
         */
        CRITICAL_ENTER ();

        /** 
         *  ������������״̬ΪDelay
         *  Set delay status for the current task */
        curr_tcb_ptr->state = TASK_DELAY;

        /** 
         *  ע��timer�Ļص�����.
         *  Register the timer callback 
         */

        /** 
         *  ����ص�����
         *  Fill out the callback to wake us up 
         */
        timerData.tcb_ptr = curr_tcb_ptr;

        /* Fill out the timer callback request structure */
        timerCb.cb_func = eTimerDelayCallback;
        timerCb.cb_data = (POINTER)&timerData;
        timerCb.cb_ticks = ticks;

        /**
         *  �洢��ʱ�ص�(��ʹ���ǲ���ʹ����)
         *  Store the timeout callback details, though we don't use it 
         */
        curr_tcb_ptr->delay_timo_cb = &timerCb;

        /** 
         *  ע�ᶨʱ���ص�
         *  Register the callback 
         */
        if (eTimerRegister (&timerCb) != EASYRTOS_OK)
        {
            /** 
             *  �˳��ٽ���
             *  Exit critical region 
             */
            CRITICAL_EXIT ();

            /** 
             *  timerע��δ�ɹ�
             *  Timer registration didn't work, won't get a callback 
             */
            status = EASYRTOS_ERR_TIMER;
        }
        else
        {
            /**
             *  �˳��ٽ���
             *  Exit critical region 
             */
            CRITICAL_EXIT ();

            /** 
             *  timerע��ɹ�
             *  Successful timer registration 
             */
            status = EASYRTOS_OK;

            /** 
             *  �������е������ӳ�,������������ʼ����
             *  Current task should now delay, schedule in another 
             */
            easyRTOSSched (FALSE);
        }
    }

    return (status);
}

void eTimerTick (void)
{
  /**
   *  Only do anything if the OS is started
   *  ��ϵͳ������ʱ�������
   */
  if (easyRTOSStarted)
  {
    /**
     *  Increment the system tick count 
     *  ����ϵͳtick����
     */
    
    systemTicks++;

    /** 
     *  Check for any callbacks that are due 
     *  ����Ƿ��к�����Ҫ�ص�.
     */
    eTimerCallbacks ();
  }
}

static void eTimerCallbacks (void)
{
  EASYRTOS_TIMER *prev_ptr = NULL, *next_ptr = NULL, *saved_next_ptr = NULL;
  EASYRTOS_TIMER *callback_list_tail = NULL, *callback_list_head = NULL;

  /*
   * Walk the list decrementing each timer's remaining ticks count and
   * looking for due callbacks.
   * ��������,�������ж�ʱ���ļ���,����Ƿ���Ҫ�ص�.
   */
  prev_ptr = next_ptr = timer_queue;
  while (next_ptr)
  {
    /** 
     *  Save the next timer in the list (we adjust next_ptr for callbacks) 
     *  ��next timer�������б���
     */
    saved_next_ptr = next_ptr->next_timer;

    /** 
     *  Is this entry due? 
     *  timer ����Ƿ���?
     */
    if (--(next_ptr->cb_ticks) == 0)
    {
      /** 
       *  Remove the entry from the timer list 
       *  ���б����Ƴ������
       */
      if (next_ptr == timer_queue)
      {
        /** 
         *  We're removing the list head 
         *  �Ƴ����б��ͷ
         */
        timer_queue = next_ptr->next_timer;
      }
      else
      {
        /** 
         *  We're removing a mid or tail timer 
         *  �Ƴ����б��л����б�β
         */
        prev_ptr->next_timer = next_ptr->next_timer;
      }

      /*
       *  Add this timer to the list of callbacks to run later when
       *  we've finished walking the list (we shouldn't call callbacks
       *  now in case they want to register new timers and hence walk
       *  the timer list.)
       *  ������ڼ�����Ҫִ�еĻص������б�.�����Ǳ����������б�֮��
       *  ��ִ��,��Ϊ�п���֮���лص���Ҫִ��.
       *
       *  We reuse the EASYRTOS_TIMER structure's next_ptr to maintain the
       *  callback list.
       *  �ظ�����EASYRTOS_TIMER�ṹ��� next_ptr ָ��������ص��б�.
       */
      if (callback_list_head == NULL)
      {
        /** 
         *  First callback request in the list 
         *  ���б�����ӵ�һ���ص�.
         */
        callback_list_head = callback_list_tail = next_ptr;
      }
      else
      {
        /** 
         *  Add callback request to the list tail 
         *  ���б�β��ӻص�
         */
        callback_list_tail->next_timer = next_ptr;
        callback_list_tail = callback_list_tail->next_timer;
      }

      /**
       *  Mark this timer as the end of the callback list 
       *  ��Ǹ�timer���ǻص��б�����һ��
       */
      next_ptr->next_timer = NULL;

      /* Do not update prev_ptr, we have just removed this one */
    }

    /**
     *  Entry is not due, leave it in there with its count decremented 
     *  �ص����û�е���.�ڴ��˳�,��������countֵ
     */
    else
    {
      /**
       * Update prev_ptr to this entry. We will need it if we want
       * to remove a mid or tail timer.
       */
      prev_ptr = next_ptr;
    }

    /* Move on to the next in the list */
    next_ptr = saved_next_ptr;
  }

  /**
   *  Check if any callbacks were due. 
   *  ����Ƿ��лص�����.
   */
  if (callback_list_head)
  {
      /**
       *  Walk the callback list 
       *  ���������ص��б�.
       */
      next_ptr = callback_list_head;
      while (next_ptr)
      {
          /*
           *  Save the next timer in the list (in case the callback
           *  modifies the list by registering again.
           *  �����б��е� next timer,�����б��޸�.
           */
          saved_next_ptr = next_ptr->next_timer;

          /** 
           *  Call the registered callback 
           *  ���ûص�
           */
          if (next_ptr->cb_func)
          {
              next_ptr->cb_func (next_ptr->cb_data);
          }

          /**
           *  Move on to the next callback in the list 
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
     *  Get the DELAY_TIMER structure pointer 
     */
    timer_data_ptr = (DELAY_TIMER *)cb_data;

    /** 
     *  �������Ƿ�ΪNULL
     *  Check parameter is valid 
     */
    if (timer_data_ptr)
    {
        /* Enter critical region */
        CRITICAL_ENTER ();

        /* Put the task on the ready queue */
        (void)tcbEnqueuePriority (&tcb_readyQ, timer_data_ptr->tcb_ptr);

        /* Exit critical region */
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
