/**  
 * ����: Roy.yu
 * ʱ��: 2016.10.28
 * �汾: V1.1
 * Licence: GNU GENERAL PUBLIC LICENSE
 */
#ifndef __EASYRTOSMM_H__
#define __EASYRTOSMM_H__

#define BSIZE  1
#define HWSIZE 2
#define WSIZE  4
#define DSIZE  8

#define HDSIZE 3
#define FTSIZE 3

#define MAX(x,y) ((x)>(y)?(x):(y))

/*
 * һ������ͷ+����+β���
 * ͷ��2.5Byte β0.5Byte
 */

/* �Է����ֽڵĴ�С��ͷ��β���д�� ��ʽΪ �����С|�����־λ */
#define PACKHF(size,alloc) ((size)|(alloc))

/* 
 * ��ȡp��ַ������ 
 * ����p��ַ������Ϊvalue
 */
#define GET(p) (*(uint8_t *)(p))
#define PUT(p,value) (*(uint8_t *)(p) = (value))

/* 
 * ��ȡp��ַ�����ݿ��С 
 * ��ȡp��ַ�����ݿ�����־λ
 */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* 
 * ��ȡ��ַΪbp�����ݿ��ͷ�ĵ�ַ 
 * ��ȡ��ַΪbp�����ݿ��β�ĵ�ַ 
 */
#define HDRP(bp) ((uint8_t*)(bp) - HDSIZE)
#define FTRP(bp) ((uint8_t*)(bp) + GETSIZE(HDRP(bp)) - (HDSIZE+FTSIZE))

/* 
 * ��ȡ��ַΪbp�����ݿ��ͷ�ĵ�ַ 
 * ��ȡ��ַΪbp�����ݿ��β�ĵ�ַ 
 */
#define NEXT_BLKP(bp) ((uint8_t*)(bp) + GETSIZE((uint8_t)(bp)-))
#define PREV_BLKP(bp) ((uint8_t*)(bp) + GETSIZE(HDRP(bp)) - (HDSIZE+FTSIZE))

#endif