/**
 * ����: Roy.yu
 * ʱ��: 2016.11.01
 * �汾: V1.1
 * Licence: GNU GENERAL PUBLIC LICENSE
 */
/* �ڲ����� */
#include "easyRTOS.h"
#include "easyRTOSkernel.h"
#include "easyRTOSMm.h"
static uint8_t *mem_heap;
static uint8_t *mem_brk;

/* �ⲿ�ɵ��ú��� */
void eMemInit(uint8_t* heapAddr,uint16_t maxHeap);
uint8_t *eMalloc(uint16_t size);
void eFree(uint8_t *addr);

/* �ڲ����� */
static uint8_t *memBlockMerge(uint8_t *bp);
static void memBlockInit(void);
static uint8_t *findFitBp(uint16_t size);
static void placeBlock(uint8_t *bp,uint16_t size);
static uint8_t *memBlockMerge(uint8_t *bp);
/*
 *  ������������ʽ��������ڴ����ռ䣬ÿ���ڴ����ͷ��head��β��foot�����м��ڴ���
 *  �ɣ�����������������ַ���ӣ�������ջ�����ѵĽ����ɶ����Ľ������ǣ��������ʾΪ
 *  һ����СΪ0���ѷ���ͷ��0/1��
 */

/*------------------------------------------------------------------------------*/
//��������ʼ�� ÿ��������Ŀ���Ҫ4Byteͷ+4Byteβ ���Է���n Byteʵ����Ҫ�ռ�n+8Byte
void eMemInit(uint8_t* heapAddr,uint16_t maxHeap)
{
  mem_heap = heapAddr;
  mem_brk  = (uint8_t*)(mem_heap+maxHeap-4);
  memBlockInit();
}

//�����ڴ���� �ҵ����ʵ���ָ���п飬����bp����û�к��ʵ��򷵻�NULL
uint8_t *eMalloc(uint16_t size)
{
  uint8_t *bp;
  if (size==0)return NULL;
  if ((bp = findFitBp(size))!=NULL)
  {
    placeBlock(bp,size);
    return bp;
  }
  else
  {
    return NULL;
  }
}

//�ͷ��ѷ�����ڴ�
void eFree(uint8_t *bp)
{
  uint16_t size = GET_SIZE(HDRP(bp));
  PUT(HDRP(bp),PACKHF(size,0));
  PUT(FTRP(bp),PACKHF(size,0));
  memBlockMerge(bp);
}

//���ʼ��
static void memBlockInit(void)
{
  uint8_t *bp = mem_heap+HDSIZE;
  uint32_t data = PACKHF((uint32_t)(mem_brk - mem_heap),0);
  static uint8_t *p;
  //��ʼ����һ��free��
  PUT(HDRP(bp),data);
  PUT(FTRP(bp),data);
  //���ý������־
  p=NEXT_BLKP(bp);
  PUT(HDRP(p),PACKHF(0,1));
}

//��Ѱ���ʴ�С�Ŀ� ֱ���������Ƿ���NULL���ҵ��򷵻ؿ��п��bp
static uint8_t *findFitBp(uint16_t size)
{
  uint8_t *bp;
  //��������Ϊ������������ �ҵ��󷵻ظ�bp
  for (bp = mem_heap+HDSIZE;!((GET_SIZE(HDRP(bp)) == 0) && (GET_ALLOC(HDRP(bp)) == 1)); bp = NEXT_BLKP(bp))
  {
    if ( (GET_SIZE(HDRP(bp)) >= (size+HDSIZE+FTSIZE)) && (!GET_ALLOC(HDRP(bp))) )
    {
      return bp;
    }
  }
  //δ�ҵ� ����NULL
  return NULL;
}

//�ָ��
static void placeBlock(uint8_t *bp,uint16_t size)
{
  uint16_t blockSize = GET_SIZE(HDRP(bp));
  uint16_t blockSizeNeed = size+HDSIZE+FTSIZE;
  PUT(HDRP(bp),PACKHF(blockSizeNeed,1));
  PUT(FTRP(bp),PACKHF(blockSizeNeed,1));
  PUT(HDRP(NEXT_BLKP(bp)),PACKHF(blockSize-blockSizeNeed,0));
  PUT(FTRP(NEXT_BLKP(bp)),PACKHF(blockSize-blockSizeNeed,0));
}

//���п�ϲ�
static uint8_t *memBlockMerge(uint8_t *bp)
{
  uint8_t prevAlloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  uint8_t nextAlloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

  //����ռ�ã���ֱ�ӷ���
  if (prevAlloc && nextAlloc)
  {
    return bp;
  }
  //ǰ�汻ռ�ã��������У����ϲ�
  else if (prevAlloc && !nextAlloc)
  {
    PUT(HDRP(bp),PACKHF(GET_SIZE(HDRP(bp))+GET_SIZE(HDRP(NEXT_BLKP(bp))),0));
    PUT(FTRP(NEXT_BLKP(bp)),PACKHF(GET_SIZE(HDRP(bp))+GET_SIZE(HDRP(NEXT_BLKP(bp))),0));
    return bp;
  }
  //���汻ռ�ã�ǰ�����У���ǰ�ϲ�
  else if (!prevAlloc && nextAlloc)
  {
    PUT(HDRP(PREV_BLKP(bp)),PACKHF(GET_SIZE(HDRP(bp))+GET_SIZE(HDRP(PREV_BLKP(bp))),0));
    PUT(FTRP(bp),PACKHF(GET_SIZE(HDRP(bp))+GET_SIZE(HDRP(PREV_BLKP(bp))),0));
    return PREV_BLKP(bp);
  }
  //���涼���У�˫��ϲ�
  else if (!prevAlloc && !nextAlloc)
  {
    PUT(HDRP(PREV_BLKP(bp)),PACKHF(GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp))),0));
    PUT(FTRP(NEXT_BLKP(bp)),PACKHF(GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp))),0));
    return PREV_BLKP(bp);
  }
  else return NULL;
}

