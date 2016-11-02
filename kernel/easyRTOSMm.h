/**
 * ����: Roy.yu
 * ʱ��: 2016.11.01
 * �汾: V1.1
 * Licence: GNU GENERAL PUBLIC LICENSE
 */
#ifndef __EASYRTOSMM_H__
#define __EASYRTOSMM_H__

#define BSIZE  1
#define HWSIZE 2
#define WSIZE  4
#define DSIZE  8

#define HDSIZE WSIZE
#define FTSIZE WSIZE

#define MAX(x,y) ((x)>(y)?(x):(y))

/*
 * һ������ͷ+����+β���
 * ͷ��3Byte+1Byte
 * β��3Byte+1Byte
 */

/* �Է����ֽڵĴ�С��ͷ��β���д�� ��ʽΪ �����С|�����־λ */
#define PACKHF(size,alloc) (((uint32_t)size<<8)|(uint32_t)(alloc))

/*
 * ��ȡp��ַ��ֵ
 * ����p��ַ��ֵΪvalue
 */
#define GET(p) (*(uint32_t *)(p))
#define PUT(p,value) (*(uint32_t *)(p) = (value))

/*
 * ��ȡp��ַ�Ŀ��С
 * ��ȡp��ַ�Ŀ�����־λ
 */
#define GET_SIZE(p)  (uint32_t)((GET(p) & ~0xff)>>8)
#define GET_ALLOC(p) (GET(p) & 0x1)

/*
 * ��ȡ��ַΪbp�Ŀ�(BLOCK)��ͷ�ĵ�ַ
 * ��ȡ��ַΪbp�Ŀ�(BLOCK)��β�ĵ�ַ
 */
#define HDRP(bp) (uint8_t*)((uint8_t*)(bp) - HDSIZE)
#define FTRP(bp) (uint8_t*)((uint8_t*)(bp) + GET_SIZE(HDRP(bp)) - (HDSIZE+FTSIZE))

/*
 * ��ȡ��ַΪbp�Ŀ���һ��ͷ�ĵ�ַ
 * ��ȡ��ַΪbp�Ŀ�ǰһ��ͷ�ĵ�ַ
 */
#define NEXT_BLKP(bp) (uint8_t*)((uint8_t*)(bp) + GET_SIZE((uint8_t*)(bp)-HDSIZE))
#define PREV_BLKP(bp) (uint8_t*)((uint8_t*)(bp) - GET_SIZE((uint8_t*)(bp)-HDSIZE-FTSIZE))

extern void eMemInit(uint8_t* heapAddr,uint16_t maxHeap);
extern uint8_t *eMalloc(uint16_t size);
extern void eFree(uint8_t *addr);
   
#endif
