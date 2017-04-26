/**
 * ����: Roy.yu
 * ʱ��: 2017.04.26
 * �汾: V1.2
 * Licence: GNU GENERAL PUBLIC LICENSE
 */
#include "easyRTOS.h"
#include "easyRTOSkernel.h"
#include "easyRTOSport.h"

/* �������е������TCB */
static EASYRTOS_TCB *curr_tcb = NULL;
/* easyRTOS������־λ */
uint8_t easyRTOSStarted = FALSE;

/* easyRTOS����������� */
EASYRTOS_TCB *tcb_readyQ = NULL;

/* easyRTOS�ж�Ƕ�׼��� */
static int easyITCnt = 0;

static EASYRTOS_TCB idleTcb;

/* ȫ�ֺ��� */
ERESULT eTaskCreat(EASYRTOS_TCB *tcb_ptr, uint8_t priority, void (*entry_point)(uint32_t), uint32_t entryParam, void* task_stack, uint32_t stackSize,const char* task_name,uint32_t taskID);
void easyRTOSStart (void);
void easyRTOSSched (uint8_t timer_tick);
ERESULT easyRTOSInit (void *idle_task_stack, uint32_t idleTaskStackSize);
ERESULT tcbEnqueuePriority (EASYRTOS_TCB **tcb_queue_ptr, EASYRTOS_TCB *tcb_ptr);
EASYRTOS_TCB *tcb_dequeue_entry (EASYRTOS_TCB **tcb_queue_ptr, EASYRTOS_TCB *tcb_ptr);
EASYRTOS_TCB *tcb_dequeue_head (EASYRTOS_TCB **tcb_queue_ptr);
EASYRTOS_TCB *tcb_dequeue_priority (EASYRTOS_TCB **tcb_queue_ptr, uint8_t priority);
EASYRTOS_TCB *eCurrentContext (void);
void eIntEnter (void);
void eIntExit (uint8_t timerTick);
/* end */

/* ˽�к��� */
static void idleTask (uint32_t param);
static void eTaskSwitch(EASYRTOS_TCB *old_tcb, EASYRTOS_TCB *new_tcb);
/* end */

/**
 * ����: ����һ��������,���������ȼ�,��������,��ջλ�ü���С,�������Ƽ����,������TCB��.
 *
 * ����:
 * ����:                                  ���:
 * EASYRTOS_TCB *tcb_ptr ����TCB          EASYRTOS_TCB *tcb_ptr ����TCB 
 * uint8_t priority �������ȼ�
 * uint32_t entryParam �������
 * void* task_stack �����ջ
 * uint32_t stackSize �����ջ��С
 * const char* task_name �������� 
 * uint32_t taskID ����ID���
 *
 * ����: ERESULT
 * EASYRTOS_OK �ɹ�
 * EASYRTOS_ERR_PARAM ����Ĳ���
 * EASYRTOS_ERR_QUEUE ���������Ready����ʧ��
 *
 * ���õĺ���:
 * archTaskContextInit (tcb_ptr, stack_top, entry_point, entryParam);
 * tcbEnqueuePriority (&tcb_readyQ, tcb_ptr);
 */
ERESULT eTaskCreat(EASYRTOS_TCB *tcb_ptr, uint8_t priority, void (*entry_point)(uint32_t), uint32_t entryParam, void* task_stack, uint32_t stackSize,const char* task_name,uint32_t taskID)
{
  ERESULT status;
  CRITICAL_STORE;
  uint8_t i = 0;
  char *p_string = NULL;
  uint8_t *stack_top = NULL;

  if ((tcb_ptr == NULL) || (entry_point == NULL) || (task_stack == NULL) || (stackSize == 0) || task_name == NULL)
  {
    /* �������� */
    status = EASYRTOS_ERR_PARAM;
  }
  else
  {
    /* ��ʼ��TCB */
    for (p_string=(char *)task_name;(*p_string)!='\0';p_string++)
    {
      tcb_ptr->taskName[i]=*p_string;
      i++;
      if (i > TASKNAMELEN)
      {
        status = EASYRTOS_ERR_PARAM;
      }
    }
    tcb_ptr->taskID = taskID;
    tcb_ptr->state = TASK_READY;
    tcb_ptr->priority = priority;
    tcb_ptr->prev_tcb = NULL;
    tcb_ptr->next_tcb = NULL;
    tcb_ptr->pended_timo_cb = NULL;
    tcb_ptr->delay_timo_cb = NULL;

    /* ��TCB�б�������������Լ����� */
    tcb_ptr->entry_point = entry_point;
    tcb_ptr->entryParam  = entryParam;;

    /* ����ջ����ڵ�ַ */
    stack_top = (uint8_t *)task_stack + (stackSize & ~(STACK_ALIGN_SIZE - 1)) - STACK_ALIGN_SIZE;

    /**
     * ����arch-specific����ȥ��ʼ����ջ,�������������һ�����������Ĵ洢����,
     * �Ա�easyRTOSTaskSwitch()�Ը�������е���.archContextSwitch() (������
     * ���ȵ�һ��ʼ�ͻᱻ����)�������������ڻָ���������Լ�������һЩ�����õ���
     * ����ֵ,�Ա����������ִ��.
     */
    archTaskContextInit (tcb_ptr, stack_top, entry_point, entryParam);

    /* �����ٽ���,����ϵͳ������� */
    CRITICAL_ENTER ();

    /* ���½����������������� */
    if (tcbEnqueuePriority (&tcb_readyQ, tcb_ptr) != EASYRTOS_OK)
    {
      /* ������ʧ�����˳��ٽ��� */
      CRITICAL_EXIT ();

      /* ���������������صĴ��� */
      status = EASYRTOS_ERR_QUEUE;
    }
    else
    {
      tcb_ptr->state = TASK_READY;
      
      /* �����˳��ٽ��� */
      CRITICAL_EXIT ();

      /* ��ϵͳ�Ѿ�����,�Ѿ���������.���������������� */
      if (easyRTOSStarted == TRUE)
          easyRTOSSched (FALSE);

      /* �������񴴽��ɹ���־ */
      status = EASYRTOS_OK;
    }
  }
  return (status);
}

/**
 * 2016.9.19 V0.2��������
 * ����: �޸�һ����������ȼ�
 * 
 * ����:
 * ����:                                            ���:
 * EASYRTOS_TCB *tcb_ptr  �����TCB                 ��.
 * uint8_t priority Ҫ���õ����ȼ�
 *
 * ����: ERESULT
 * EASYRTOS_OK �ɹ�
 * EASYRTOS_ERR_PARAM ����Ĳ���
 *
 * ���õĺ���:
 * tcb_dequeue_entry (&tcb_readyQ, tcb_ptr);
 * tcbEnqueuePriority (&tcb_readyQ, tcb_ptr);
 */
ERESULT eSetTaskPriority(EASYRTOS_TCB *tcb_ptr,uint8_t priority)
{
  CRITICAL_STORE;
  ERESULT status = EASYRTOS_OK;
  EASYRTOS_TCB *new_tcb = NULL;
  /* �����ٽ���,����ϵͳ������� */
  CRITICAL_ENTER ();
  new_tcb = tcb_dequeue_entry (&tcb_readyQ, tcb_ptr);
  tcb_ptr->priority = priority;
  if (new_tcb!=NULL)
  {
    status = tcbEnqueuePriority (&tcb_readyQ, tcb_ptr);
  }
  /*�˳��ٽ���*/
  CRITICAL_EXIT ();
  return (status);
}

/**
 * ����: ϵͳ��ʼ��,����һ��Idle Task.
 * 
 * ����:
 * ����:                                            ���:
 * void *idle_task_stack  ���������ջλ��          ��.
 * uint32_t idleTaskStackSize ���������ջ��С
 *
 * ����: ERESULT
 * EASYRTOS_OK �ɹ�
 * EASYRTOS_ERR_PARAM ����Ĳ���
 * EASYRTOS_ERR_QUEUE ���������Ready����ʧ��
 *
 * ���õĺ���:
 * eTaskCreat(&idleTcb,255,idleTask,0,idle_task_stack,idleTaskStackSize,"IDLE",0);
 */
ERESULT easyRTOSInit (void *idle_task_stack, uint32_t idleTaskStackSize)
{
    ERESULT status;

    /* ��ʼ������ */
    curr_tcb = NULL;
    tcb_readyQ = NULL;
    easyRTOSStarted = FALSE;

    /* ���������� */
    status = eTaskCreat(&idleTcb,
                 255,
                 idleTask,
                 0,
                 idle_task_stack,
                 idleTaskStackSize,
                 "IDLE",
                 0);
    
    return (status);
}

/**
 * ����: ϵͳ����,�������ȼ���ߵ�����
 * 
 * ����:
 * ����:                    ���:
 * ��.                      ��.
 * 
 * ����: void
 *
 * ���õĺ���:
 * tcb_dequeue_priority (&tcb_readyQ, 255);
 * archFirstTaskRestore (new_tcb);
 */
void easyRTOSStart (void)
{
    EASYRTOS_TCB *new_tcb;

    /* ��λϵͳ������־λ,�����־λ��Ӱ��eTaskCreate(),ʹ�øú����������������� */
    easyRTOSStarted = TRUE;

    /* ȡ�����ȼ���ߵ�TCB,������������.��û�д����κ���������������ȼ���͵Ŀ����� */
    new_tcb = tcb_dequeue_priority (&tcb_readyQ, 255);
    if (new_tcb)
    {
      curr_tcb = new_tcb;
			new_tcb->state = TASK_RUN;
      /* �ָ������е�һ������ */
      archFirstTaskRestore (new_tcb);

    }
    else
    {
      /* û�ҵ��������е����� */
    }
}

/**
 * ����: ����������.
 * 1.����false:������Ready״̬��Run��������,ֻ�����ȼ����ڵ�ǰ����Ĳ�����ռ
 * 2.����true:������Ready״̬��Run��������,��ͬ���߸����ȼ��Ŀ�����ռ��ǰ����
 *
 * ����:
 * ����:                                            ���:
 * uint8_t timer_tick  ��ʱ���жϵ���               ��.
 *
 * ����: void
 * 
 * ���õĺ���:
 * tcb_dequeue_head (&tcb_readyQ);
 * eTaskSwitch (curr_tcb, new_tcb);
 * tcb_dequeue_priority (&tcb_readyQ, (uint8_t)lowest_pri);
 * tcbEnqueuePriority (&tcb_readyQ, curr_tcb);
 */
void easyRTOSSched (uint8_t timer_tick)
{
    CRITICAL_STORE;
    EASYRTOS_TCB *new_tcb = NULL;
    int16_t lowest_pri;

    /* ���RTOS�Ƿ��� */
    if (easyRTOSStarted == FALSE)
    {
      return;
    }

    /* �����ٽ��� */
    CRITICAL_ENTER();

    if (curr_tcb->state != TASK_RUN)
    {
      /* ���Ѿ����������������ȡ��һ��.�����б�Ȼ��idleTask */
      new_tcb = tcb_dequeue_head (&tcb_readyQ);

      /* �л��������� */
      eTaskSwitch (curr_tcb, new_tcb);
    }

    /* ��������ȻΪ����̬,�����Ƿ������������Ѿ�����,����Ҫ���� */
    else
    {
      /* ����������ȵ����ȼ� */
      if (timer_tick == TRUE)
      {
        /* ��ͬ���߸������ȼ������������ռ */
        lowest_pri = (int16_t)curr_tcb->priority;
      }
      else if (curr_tcb->priority > 0)
      {
        /* ֻ�и��ߵ����ȼ�������ռCPU,������ȼ�Ϊ0 */
        lowest_pri = (int16_t)(curr_tcb->priority - 1);
      }
      else
      {
        /* Ŀǰ�����ȼ��Ѿ������,�������κ��߳���ռ */
        lowest_pri = -1;
      }

      /* ����Ƿ���е��� */
      if (lowest_pri >= 0)
      {
        /* ����Ƿ��в����ڸ������ȼ������� */
        new_tcb = tcb_dequeue_priority (&tcb_readyQ, (uint8_t)lowest_pri);

        /* ���ҵ���Ӧ����,����֮ */
        if (new_tcb)
        {
          /* ���������е��������������� */
          if (tcbEnqueuePriority (&tcb_readyQ, curr_tcb) == EASYRTOS_OK)
          {
            curr_tcb->state = TASK_READY;
          }
          /* �л��������� */
          eTaskSwitch (curr_tcb, new_tcb);
        }
      }
    }

    /* �˳��ٽ��� */
    CRITICAL_EXIT ();
}

/**
 * ����: ���������ȼ�������TCB�����б�
 * 
 * ����:
 * ����:                                           ���:
 * EASYRTOS_TCB **tcb_queue_ptr ������Ķ���       EASYRTOS_TCB **tcb_queue_ptr ������Ķ���
 * EASYRTOS_TCB *tcb_ptr Ҫ���������TCB
 *
 * ����: ERESULT
 * EASYRTOS_OK �ɹ�
 * EASYRTOS_ERR_PARAM ����Ĳ���
 *
 * ���õĺ���:
 * ��.
 */
ERESULT tcbEnqueuePriority (EASYRTOS_TCB **tcb_queue_ptr, EASYRTOS_TCB *tcb_ptr)
{
    ERESULT status;
    EASYRTOS_TCB *prev_ptr, *next_ptr;

    /* ������� */
    if ((tcb_queue_ptr == NULL) || (tcb_ptr == NULL))
    {
      
      /* ���ش��� */
      status = EASYRTOS_ERR_PARAM;
    }
    else
    {
      
      /* ������������ */
      prev_ptr = next_ptr = *tcb_queue_ptr;
      do
      {
        
        /** 
         *  ��������:
         *   next_ptr = NULL (�Ѿ������е�ĩβ��.)
         *   ��һ��TCB�����ȼ���Ҫ�����TCB���ȼ���.
         */
        if ((next_ptr == NULL) || (next_ptr->priority > tcb_ptr->priority))
        {
          
          /* �����ȼ�Ϊ�������,�򽫸�TCB��Ϊ���е�ͷ */
          if (next_ptr == *tcb_queue_ptr)
          {
            *tcb_queue_ptr = tcb_ptr;
            tcb_ptr->prev_tcb = NULL;
            tcb_ptr->next_tcb = next_ptr;
            if (next_ptr)
                next_ptr->prev_tcb = tcb_ptr;
          }
          
          /* �ڶ����в���TCB������ĩβ���� */
          else
          {
            tcb_ptr->prev_tcb = prev_ptr;
            tcb_ptr->next_tcb = next_ptr;
            prev_ptr->next_tcb = tcb_ptr;
            if (next_ptr)
                next_ptr->prev_tcb = tcb_ptr;
          }

          /* ��ɲ���,�˳�ѭ�� */
          break;
        }
        else
        {
          /* ����λ�ò���,������һ��λ�� */
          prev_ptr = next_ptr;
          next_ptr = next_ptr->next_tcb;
        }

      }
      while (prev_ptr != NULL);

      /* �ɹ� */
      status = EASYRTOS_OK;
    }
    return (status);
}

/**
 * ����: ��ȡ��ǰ���е�����TCB
 * 
 * ����:
 * ����:                     ���:
 * ��                        ��.
 *
 * ����: EASYRTOS_TCB*
 * ��ǰ��������TCBָ��
 *  
 * ���õĺ���:
 * ��.
 */
EASYRTOS_TCB *eCurrentContext (void)
{
    if (easyITCnt == 0)
        return (curr_tcb);
    else
        return (NULL);
}

/**
 * ����: ȡ���б�ͷ������TCB
 * 
 * ����:
 * ����:                                           ���:
 * EASYRTOS_TCB **tcb_queue_ptr ��ȡ���б�         EASYRTOS_TCB **tcb_queue_ptr ��ȡ���б�
 *
 * ����: EASYRTOS_TCB *
 * �����б�ͷ������TCBָ��
 *
 * ���õĺ���:
 * ��.
 */
EASYRTOS_TCB *tcb_dequeue_head (EASYRTOS_TCB **tcb_queue_ptr)
{
    EASYRTOS_TCB *ret_ptr;

    /* ������� */
    if (tcb_queue_ptr == NULL)
    {
        /* ����NULL */
        ret_ptr = NULL;
    }
    else if (*tcb_queue_ptr == NULL)
    {
        /* ����NULL */
        ret_ptr = NULL;
    }
    /* ���ض���ͷ,�������Ƴ����� */
    else
    {
        ret_ptr = *tcb_queue_ptr;
        *tcb_queue_ptr = ret_ptr->next_tcb;
        if (*tcb_queue_ptr)
            (*tcb_queue_ptr)->prev_tcb = NULL;
        ret_ptr->next_tcb = ret_ptr->prev_tcb = NULL;
    }
    return (ret_ptr);
}

/**
 * ����: ���б����Ƴ�ָ��������TCB
 * 
 * ����:
 * ����:                                           ���:
 * EASYRTOS_TCB **tcb_queue_ptr ��ȡ���б�         EASYRTOS_TCB **tcb_queue_ptr ��ȡ���б� 
 * EASYRTOS_TCB *tcb_ptr ��Ҫ�Ƴ�������TCB
 * 
 * ����: EASYRTOS_TCB *
 * �����Ƴ���TCBָ��
 *
 * ���õĺ���:
 * ��.
 */
EASYRTOS_TCB *tcb_dequeue_entry (EASYRTOS_TCB **tcb_queue_ptr, EASYRTOS_TCB *tcb_ptr)
{
    EASYRTOS_TCB *ret_ptr, *prev_ptr, *next_ptr;

    /* ������� */
    if (tcb_queue_ptr == NULL)
    {
        /* ����NULL */
        ret_ptr = NULL;
    }
    else if (*tcb_queue_ptr == NULL)
    {
        /* ����NULL */
        ret_ptr = NULL;
    }
    /* �ҵ�,�Ƴ�������ָ����tcb */
    else
    {
        ret_ptr = NULL;
        prev_ptr = next_ptr = *tcb_queue_ptr;
        while (next_ptr)
        {
          
            /* �Ƿ�������������TCB? */
            if (next_ptr == tcb_ptr)
            {
              
                /* Ѱ�ҵ�TCBΪ����ͷ */
                if (next_ptr == *tcb_queue_ptr)
                {
                    /* �Ƴ�����ͷ */
                    *tcb_queue_ptr = next_ptr->next_tcb;
                    if (*tcb_queue_ptr)
                        (*tcb_queue_ptr)->prev_tcb = NULL;
                }
                
                /* Ѱ�ҵ�TCB�ڶ����м��ĩβ.*/
                else
                {
                    prev_ptr->next_tcb = next_ptr->next_tcb;
                    if (next_ptr->next_tcb)
                        next_ptr->next_tcb->prev_tcb = prev_ptr;
                }
                ret_ptr = next_ptr;
                ret_ptr->prev_tcb = ret_ptr->next_tcb = NULL;
                break;
            }

            /* �ƶ��������е���һ��TCB */
            prev_ptr = next_ptr;
            next_ptr = next_ptr->next_tcb;
        }
    }
    return (ret_ptr);
}

/**
 * ����: ���б���ȡ�����ȼ�����priority������TCB,ֻ����һ��
 * 
 * ����:
 * ����:                                           ���:
 * EASYRTOS_TCB **tcb_queue_ptr ��ȡ���б�         ��.
 * uint8_t priority
 * 
 * ����: EASYRTOS_TCB *
 * �����Ƴ���TCBָ��
 *
 * ���õĺ���:
 * ��.
 */
EASYRTOS_TCB *tcb_dequeue_priority (EASYRTOS_TCB **tcb_queue_ptr, uint8_t priority)
{
    EASYRTOS_TCB *ret_ptr;

    /* ������� */
    if (tcb_queue_ptr == NULL)
    {
      ret_ptr = NULL;
    }
    else if (*tcb_queue_ptr == NULL)
    {
      ret_ptr = NULL;
    }
    
    /* ����Ƿ��к������ȼ������� */
    else if ((*tcb_queue_ptr)->priority <= priority)
    {
      ret_ptr = *tcb_queue_ptr;
      *tcb_queue_ptr = (*tcb_queue_ptr)->next_tcb;
      if (*tcb_queue_ptr)
      {
        (*tcb_queue_ptr)->prev_tcb = NULL;
        ret_ptr->next_tcb = NULL;
      }
    }
    else
    {
      /* û�к�������  */      
      ret_ptr = NULL;
    }
    return (ret_ptr);
}

/**
 * ����: �л�2������������
 * 
 * ����:
 * ����:                                           ���:
 * EASYRTOS_TCB *old_tcb ���л�������              ��.
 * EASYRTOS_TCB *new_tcb �л���������
 * 
 * ����: void
 *
 * ���õĺ���:
 * archContextSwitch (old_tcb, new_tcb);
 */
static void eTaskSwitch(EASYRTOS_TCB *old_tcb, EASYRTOS_TCB *new_tcb)
{
    
    /* ���еĳ����л����µ�����,���Խ�����״̬�л���RUN */
    new_tcb->state = TASK_RUN;

    /* ����µ������Ƿ���Ŀǰ���е�����,�����,����Ҫ�����л� */
    if (old_tcb != new_tcb)
    {
        curr_tcb = new_tcb;

        /* ���������л����� */
        archContextSwitch (old_tcb, new_tcb);
    }
}

/**
 * ����: �����жϵ���,˵�����������ж���
 * 
 * ����:
 * ����:                ���:             
 * ��.                  ��.
 * 
 * ����: void
 *
 * ���õĺ���:
 * ��.
 */
void eIntEnter (void)
{
    /* �����жϼ��� */
    easyITCnt++;
}

/**
 * ����: �˳��жϵ���,֮����õ�����
 * 
 * ����:
 * ����:                ���:             
 * uint8_t timerTick    ��.
 * 
 * ����: void
 *
 * ���õĺ���:
 * easyRTOSSched (timerTick);
 */
void eIntExit (uint8_t timerTick)
{
    /* �˳��ж�ʱ���� */
    easyITCnt--;

    /* �˳��ж�ʱ���õ����� */
    easyRTOSSched (timerTick);
}

/**
 * ����: idleTask����
 * 
 * ����:
 * ����:                ���:             
 * uint32_t param       ��.
 * 
 * ����: void
 *
 * ���õĺ���:
 * ��
 */
static void idleTask (uint32_t param)
{
  /* �������������� */
  param = param;

  while (1)
  {
     /* �պ���ִ�� */
  }
}
