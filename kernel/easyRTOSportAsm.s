 /**
 * ����: Roy.yu
 * ʱ��: 2016.11.01
 * �汾: V1.1
 * Licence: GNU GENERAL PUBLIC LICENSE
 */
 NAME EASYRTOSPORTASM
 SECTION .near_func.text:code

#include "vregs.inc"

;void archFirstThreadRestore (EASYRTOS_TCB *new_tcb_ptr)
  PUBLIC archFirstTaskRestore
archFirstTaskRestore:
  ; Parameter locations:
  ; new_tcb_ptr = X register (word-width)
  ; POINTER sp_save_ptr;
  ; (X) = *sp_save_ptr
  ldw X,(X)

  ; �л�ջָ�뵽������Ķ�ջ
  ldw SP,X

  ; ?b15 - ?b8 ��ջ
  POP ?b15
  POP ?b14
  POP ?b13
  POP ?b12
  POP ?b11
  POP ?b10
  POP ?b9
  POP ?b8

  ; RET PCH=SP++;PCL=SP++;
  ret

  ;void archContextSwitch (EASYRTOS_TCB *old_tcb_ptr, EASYRTOS_TCB *new_tcb_ptr)
    PUBLIC archContextSwitch
archContextSwitch:

  ; Parameter locations :
  ;   old_tcb_ptr = X register (word-width)
  ;   new_tcb_ptr = Y register (word-width)

  ; ��������λ��:
  ;   old_tcb_ptr = X �Ĵ��� (16bit)
  ;   new_tcb_ptr = Y �Ĵ��� (16bit)
  ; STM8 CPU Registers:
  ; STM8��CPU�Ĵ���

  ; һЩ�Ĵ����Ѿ����Զ�������,��ֻ��Ҫ���Ᵽ����Щ��ϣ�����ƻ��ļĴ���.

  PUSH ?b8
  PUSH ?b9
  PUSH ?b10
  PUSH ?b11
  PUSH ?b12
  PUSH ?b13
  PUSH ?b14
  PUSH ?b15
  
  ; ��Y�Ĵ����и��Ƴ�new_tcb_ptr,������?b0��
  ldw ?b0, Y

  ; ���浱ǰ��ջָ�뵽old_tcb_ptr��
  ldw Y, SP    ; ����ǰ��ջָ��(���л�������)���浽Y�Ĵ�����
  ldw (X), Y   ; ��ջָ�뱣����(X)�ĵ�ַ��,

  ldw X,?b0
  ldw X,(X)

  
  ldw SP,X

  ; IAR����Ҫ����ͻָ� ?b8 to ?b15
  POP ?b15
  POP ?b14
  POP ?b13
  POP ?b12
  POP ?b11
  POP ?b10
  POP ?b9
  POP ?b8

  ret

  end
  