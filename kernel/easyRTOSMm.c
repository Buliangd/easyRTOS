/**  
 * ����: Roy.yu
 * ʱ��: 2016.10.28
 * �汾: V1.1
 * Licence: GNU GENERAL PUBLIC LICENSE
 */
/* �ڲ����� */
static uint8_t *mem_heap;
static uint8_t *mem_brk;
/* �ⲿ�ɵ��ú��� */
void memInit(uint16_t maxHeap);

/* �ڲ����� */
void memInit(uint16_t maxHeap)
{
  mem_heap = (uint8_t*)0x000011;
  mem_brk  = (uint8_t*)(mem_heap+maxHeap);
}