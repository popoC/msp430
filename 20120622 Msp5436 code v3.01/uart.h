#ifndef UART_H
#define UART_H
#include "msp430x54x.h"
#include "string.h"


#define  COM1 0x01
#define  COM2 0x02
#define  COM3 0x04
#define  COM4 0x08

#define  COM_BUF_Size 128

#ifdef uart_CPP
    #define AUTOEXT
#else 
    #define AUTOEXT  extern
#endif 
AUTOEXT  int control_mode;
AUTOEXT  int UART_COM1_RX_count;
AUTOEXT  unsigned char UART_COM1_RX_BUF[COM_BUF_Size];
AUTOEXT  int UART_COM2_RX_count;
AUTOEXT  unsigned char UART_COM2_RX_BUF[COM_BUF_Size];
AUTOEXT  int UART_COM3_RX_count;
AUTOEXT  unsigned char UART_COM3_RX_BUF[COM_BUF_Size];
AUTOEXT  int UART_COM4_RX_count;
AUTOEXT  unsigned char UART_COM4_RX_BUF[COM_BUF_Size];
AUTOEXT  char COM2_Command[COM_BUF_Size];
AUTOEXT  char COM3_Command[COM_BUF_Size];
#undef AUTOEXT





typedef unsigned long       DWORD;
typedef char                BYTE;



void UART_Init(BYTE com);
void UART_SendByte(unsigned char data,BYTE com);
void UART_SendStr(char *str,BYTE com);



void RS232_Send_Char(char *str,int count,BYTE com);





#endif