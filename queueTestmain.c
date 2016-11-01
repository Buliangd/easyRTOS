/**  
 * ����: Roy.yu
 * ʱ��: 2016.8.23
 * �汾: V0.1
 * Licence: GNU GENERAL PUBLIC LICENSE
 */
#include "easyRTOS.h"
#include "easyRTOSkernel.h"
#include "easyRTOSport.h"
#include "easyRTOSTimer.h"
#include "easyRTOSSem.h"
#include "easyRTOSQueue.h"
#include "easyRTOSMm.h"

/* ���� malloc() */
//#include "stdlib.h"

#define IDLE_STACK_SIZE_BYTES  128
#define TEST_STACK_SIZE_BYTES  128
#define QUEUE_STACK_SIZE_BYTES 128

#define HEAP_DEEP 5000
NEAR static uint8_t heapRoom[HEAP_DEEP];

EASYRTOS_TCB testTcb;
EASYRTOS_TCB queueTcb;
EASYRTOS_QUEUE qQueue;
void testTaskFunc (uint32_t param);
void queueTaskFunc (uint32_t param);

typedef struct test{
  uint8_t delayTime1;
  uint8_t delayTime2;
}testStruct;

testStruct *a[2];

int main( void )
{
  ERESULT status;
  
  uint8_t *idleTaskStack;
  uint8_t *testTaskStack;
  uint8_t *queueTaskStack;
  
  /* ����CPUΪ�ڲ�ʱ�� 16M*/
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
  
  eMemInit(heapRoom,HEAP_DEEP);
  
  idleTaskStack = eMalloc(IDLE_STACK_SIZE_BYTES);
  testTaskStack = eMalloc(TEST_STACK_SIZE_BYTES);
  queueTaskStack= eMalloc(QUEUE_STACK_SIZE_BYTES);
  /* ϵͳ��ʼ�� */
  status = easyRTOSInit(idleTaskStack, IDLE_STACK_SIZE_BYTES);
  
  if (status == EASYRTOS_OK)
  {
      /* ʹ��ϵͳʱ�� */
      archInitSystemTickTimer();
      
      /* �������� */            
      status += eTaskCreat(&testTcb,         
                   9, 
                   testTaskFunc, 
                   0,
                   testTaskStack,
                   TEST_STACK_SIZE_BYTES,
                   "TEST",
                   1);
      status += eTaskCreat(&queueTcb,
                   10, 
                   queueTaskFunc, 
                   1,
                   queueTaskStack,
                   QUEUE_STACK_SIZE_BYTES,
                   "QUEUE",
                   2);
      
      /* �������� */
      qQueue = eQueueCreate ( (void *)a, sizeof(testStruct *), 2);
      if (status == EASYRTOS_OK)
      {
        /* ����ϵͳ */
        easyRTOSStart();
      }
  }
  return 0;
}

void testTaskFunc (uint32_t param)
{
  testStruct *test1;
  GPIO_Init(GPIOD, GPIO_PIN_2, GPIO_MODE_OUT_PP_HIGH_FAST);
  while (1)
  {
    if (eQueueTake (&qQueue, 0, (void *)&test1) == EASYRTOS_OK)
    {
      GPIO_WriteReverse(GPIOD, GPIO_PIN_2);
      eTimerDelay(DELAY_S(test1->delayTime1+test1->delayTime2));
    }
  }
}

void queueTaskFunc (uint32_t param)
{
  testStruct test2;
  test2.delayTime1 = param;
  test2.delayTime2 = param;
  testStruct *temp =&test2;
  while (1)
  {
    temp = &test2;
    if (eQueueGive(&qQueue, 0, (void *)&temp) == EASYRTOS_OK)
    {
      
    }
  }
}