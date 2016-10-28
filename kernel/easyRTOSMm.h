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

#define MAX(x,y) ((x)>(y)?(x):(y))

/*
 * һ������ͷ+����+β���
 * ͷ��2.5Byte 
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
 * ��ȡp��ַ�����ݿ��С 
 * ��ȡp��ַ�����ݿ�����־λ
 */
#define HDRP(bp) ((uint8_t*)(bp) & ~0x7)
#define FTRP(bp) (GET(p) & 0x1)

#endif