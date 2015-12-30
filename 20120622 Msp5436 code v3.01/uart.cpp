#include "main.h"
#include "uart.h"
#include "msp430x54x.h"


   int control_mode;
   int UART_COM1_RX_count;
   unsigned char UART_COM1_RX_BUF[COM_BUF_Size];
   int UART_COM2_RX_count;
   unsigned char UART_COM2_RX_BUF[COM_BUF_Size];
   int UART_COM3_RX_count;
   unsigned char UART_COM3_RX_BUF[COM_BUF_Size];
   int UART_COM4_RX_count;
   unsigned char UART_COM4_RX_BUF[COM_BUF_Size];

   bool Check_Command_flag; 
  extern int OUTPUT_MODE ;
  extern char string[150];
  extern bool StartRecord;
  extern bool Set_RTC_Flag;
extern unsigned char RTC_Serial[7];
  
//------------------------------------------------------------------------------
void UART_Init(BYTE com){
   //須要先啟動 XT2 20MHZ
  if( (com&COM1) == COM1 ){
          
      P3SEL = 0x30;                             // P3.4,5 = USCI_A0 TXD/RXD
      UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**

      UCA0CTL1 |= UCSSEL_2;                     // CLK = SCLK = 8MHZ
      UCA0BR0 = 0xAD;                           //   (see User's Guide)  20M/115200 = 173
      UCA0BR1 = 0x00;                           //

      UCA0MCTL = UCBRS_3+UCBRF_0;               // Modulation UCBRSx=2, UCBRFx=0

      UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
      UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
  }
  if( (com&COM2) == COM2 ){

      P5SEL |= 0xc0;                             // P5.6,7 = USCI_A0 TXD/RXD
      UCA1CTL1 |= UCSWRST;                      // **Put state machine in reset**

      UCA1CTL1 |= UCSSEL_2;                     // CLK = SCLK
      UCA1BR0 = 0xAD; 
      UCA1BR1 = 0x00;                           //
      UCA1MCTL = UCBRS_3+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0

      UCA1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
      UCA1IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
  }

   if( (com&COM3) == COM3 ){

      P9SEL |= 0x30;                             // P9.4,5 = USCI_A0 TXD/RXD
      UCA2CTL1 |= UCSWRST;                      // **Put state machine in reset**

      UCA2CTL1 |= UCSSEL_2;                     // CLK = sCLK
      UCA2BR0 = 0x8D;                           // 20Mhz/2400=8333 (see User's Guide) for seascan
      UCA2BR1 = 0x20;                           //
      UCA2MCTL = UCBRS_3+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0

      UCA2CTL0 = 0;
      UCA2CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
      UCA2IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
  }
  UART_COM1_RX_count = 0;
  UART_COM2_RX_count = 0;
  UART_COM3_RX_count = 0;
  
}
//----------------------------------------------------------------------------
#pragma vector=USCI_A0_VECTOR  //COM1
__interrupt void USCI_A0_ISR(void)
{
  switch(__even_in_range(UCA0IV,4))
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector 2 - RXIFG
//    while (!(UCA0IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
        UART_COM1_RX_BUF[UART_COM1_RX_count] = UCA0RXBUF;
        UART_COM1_RX_count++;
    if(UART_COM1_RX_count>=COM_BUF_Size)UART_COM1_RX_count=0;
    break;
  case 4:break;                             // Vector 4 - TXIFG
  default: break;  
  }
    __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
}
//----------------------------------------------------------------------------
#pragma vector=USCI_A1_VECTOR //COM2
__interrupt void USCI_A1_ISR(void)
{
  
  //---命令格式    @xm..........$
  //---            @ 開始字元
  //---            x 令命字數
  //---            m 命令模式
  //---            .....  命令
  //---            $ 結束字元   
  
  switch(__even_in_range(UCA1IV,4))
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector 2 - RXIFG
//    while (!(UCA1IFG&UCTXIFG));           // USCI_A1 TX buffer ready?
//    UCA1TXBUF = UCA1RXBUF;                // TX -> RXed character
    UART_COM2_RX_BUF[UART_COM2_RX_count] = UCA1RXBUF;
    UART_COM2_RX_count++;
    if(UART_COM2_RX_count>=COM_BUF_Size)UART_COM2_RX_count=0;   
    
    if( (UART_COM2_RX_BUF[0]=='@')){        //- 命令拆解
       if((UART_COM2_RX_count>0)&&(UART_COM2_RX_count==UART_COM2_RX_BUF[1])&&(UART_COM2_RX_BUF[UART_COM2_RX_count-1]=='$')){

         switch(UART_COM2_RX_BUF[2]){
         case 'T':
           OUTPUT_MODE = LOGGER_TIME;
           break;
         case 'A':
           OUTPUT_MODE = AD_DATA;
           break;
         case 'S':
           OUTPUT_MODE = 0;           
           StartRecord = true;
           break;
         case 'R':
           if((UART_COM2_RX_BUF[3]=='E')&&(UART_COM2_RX_BUF[4]=='S')&&(UART_COM2_RX_BUF[5]=='E')&&(UART_COM2_RX_BUF[6]=='T')){
             OUTPUT_MODE = RE_SET;           
           }
           break;   
         case 't':
            memcpy(RTC_Serial,&(UART_COM2_RX_BUF[3]),7);
            Set_RTC_Flag = true;
            
           break; 
           
           
         }
       UART_COM2_RX_count = 0;  
       }
     }
     else{
       UART_COM2_RX_count = 0;
     }

     
     
    if(UART_COM2_RX_count>0){
     if(UART_COM2_RX_BUF[UART_COM2_RX_count-1]=='$')UART_COM2_RX_count = 0;
    }

    

      
    break;
  case 4:break;                             // Vector 4 - TXIFG
  default: break;  
  }
 //   __bic_SR_register_on_exit(LPM3_bits); // Exit LPM0
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#pragma vector=USCI_A2_VECTOR  //COM3
__interrupt void USCI_A2_ISR(void)
{
  switch(__even_in_range(UCA2IV,4))
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector 2 - RXIFG
    break;
  case 4:break;                             // Vector 4 - TXIFG
  default: break;  
  }
 //   __bic_SR_register_on_exit(LPM3_bits); // Exit LPM0
}
//----------------------------------------------------------------------------
void UART_SendByte(unsigned char data,BYTE com)
{
  if((com&COM1) == COM1){
    while (!(UCA0IFG&UCTXIFG));             // USCI_A1 TX buffer ready?
    UCA0TXBUF = data;
  }
  if((com&COM2) == COM2){
    while (!(UCA1IFG&UCTXIFG));             // USCI_A1 TX buffer ready?
    UCA1TXBUF = data;
  }
  if((com&COM3) == COM3){
    while (!(UCA2IFG&UCTXIFG));             // USCI_A1 TX buffer ready?
    UCA2TXBUF = data;
  }
  
  
}
void UART_SendStr(char *str,BYTE com)
{
        while (*str != '\0')
        {
            UART_SendByte(*str++,com);	          // 發送數據
        }
 //       UART_COM1_RX_count=0;
 //       UART_COM2_RX_count=0;                     // 20110310修改 發送命令後清除BUFF內資料
 //       UART_COM3_RX_count=0;   
        
}
void RS232_Send_Char(char *str,int count,BYTE com)
{
        for(int i=0;i<count;i++)
        {
            UART_SendByte(*str++,com);	          // 發送數據
        }
}
//-------------------------------------------------------------------------------
