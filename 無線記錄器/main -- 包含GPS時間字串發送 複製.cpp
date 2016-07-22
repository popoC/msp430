//-------------P6---------------------------------------------------------------
//       P1.5
//         |                           P1.7 GPS PPS_IN<---    
//      ___↓_                    __________|______                   _____
//     |      |P1.3  ->  P6.1    |                |<---  Rx        |     |
//     |      |P1.4  ->  P6.2    |                |--->  Tx (COM2) |  PC |
//     |      |P1.1  <-  P6.3    |                |--->  Gnd       |_____|
//     | 2013 |P1.2  <-  P6.4    |    430f5436    |                 ______        
//     |      |                  |                |<---Tx          |     |
//     |      |                  |                |--->Rx  (COM1)  |GPS  |
//     |______|P1.7  <-  P6.7    |________________|--->Gnd         |_____|
//------------------------------------------------------------------------------
// 2016 0104 for 土石流~  20Mhz - 100Hz  
#include  <msp430x54xA.h>
#include <stdio.h>
//#include "main.h"
#include "hp_time.h"
#include "string.h"

#define  COM1 0x01
#define  COM2 0x02

void Crystal_Init(); 
void UART_Init(char com);
void RS232_Send_Char(char *str,int count,char com);

int UART_COM1_RX_count,UART_COM2_RX_count;
unsigned char UART_COM2_RX_BUF[128];

char COM1_BUFFER[128];    // -- 115200 msp430
char COM1_REC_BUFFER[128];    // -- receive buffer

//static hptime_t sysTime[2];       //--   1s = 1000000 counter
static hptime_t sysTime,sysTime_Buffer;       //--   1s = 1000000 counter
static int  sysTime_flag = 0;

//bool sysTime_flag = false;

#define BUF_SIZE 10


char GPS_Time_String[] = "2000/12/22 10:12:22.000000";

long int CloCk_10KHz =0;

void findStrPoint(char *a,char *ans,char feature,int n);



hptime_t get_hp_Gps_time(char *tstr){
   char gpsinfo[10];
   hptime_t s1;

           findStrPoint(tstr,gpsinfo,',',2);
        
            GPS_Time_String[11] =  gpsinfo[0];
            GPS_Time_String[12] =  gpsinfo[1];
            GPS_Time_String[14] =  gpsinfo[2];
            GPS_Time_String[15] =  gpsinfo[3];
            GPS_Time_String[17] =  gpsinfo[4];
            GPS_Time_String[18] =  gpsinfo[5];
           
           findStrPoint(tstr,gpsinfo,',',10);

            GPS_Time_String[2] =  gpsinfo[4];
            GPS_Time_String[3] =  gpsinfo[5];
            GPS_Time_String[5] =  gpsinfo[2];
            GPS_Time_String[6] =  gpsinfo[3];
            GPS_Time_String[8] =  gpsinfo[0];
            GPS_Time_String[9] =  gpsinfo[1];
          //  sprintf(&GPS_Time_String[20],"%04d",CloCk_10KHz);
            s1 = ms_timestr2hptime(GPS_Time_String);       
           //s1 = ms_timestr2hptime("1980/12/22 10:12:22.016803");       
           return s1;       
  
}

//----------for rs232-----------------------------------------------------------
char COM2_Command[128];
char string[150];
void delay_ms(int del);
char out_checksum = 0;
//----------for ADS1222---------------------------------------------------------
unsigned long DATA[4]={0} ;                        //資料暫存區
bool ADS1222_Interrupt_Flag = false;
   void ADS1222Init();
   void ADS1222_STANDBY_MODE();
    int ADS1222_SELF_CALIBRATION();
                     int First_Data = 0;
                     int DRDY_ready = 0;
                     int BITN[8] = {0x0001,0x0002,0x0004,0x0008,0x0010,0x0020,0x0040,0x0080};
  
                 
//-----------for GPS------------------------------------------------------------
                     
                    
                    
                     
//------------------------------------------------------------------------------                     
int GPS_count = 0;
//int nTX1_Len = 16;

unsigned long textpoint = 0;

void main(void)
{
   WDTCTL = WDTPW + WDTHOLD;                    // Stop WDT
   
   P6DIR |= (BIT3+BIT4+BIT7);      //out   BIT7 很重要
   P6DIR &= ~(BIT1+BIT2+BIT6);     //input
   P6OUT &= ~(BIT4+BIT3+BIT7); 
   
   P1DIR = (BIT0 +BIT1 + BIT6) ;
   P1OUT |= BIT0;                           //  

   
   P1DIR &= ~BIT7;
   P1IE  = 0 ;                           //
   P1IES =0;                             //
   P1IFG =0;                             //
   P1IE  |= BIT7;                        // Set P1.7 interrupt
   P1IES &= ~BIT7;                        // 選擇中斷模式(0 由Lo變Hi觸發 )
   
   
   
   
   Crystal_Init();                            // 震盪器初始化
   UART_Init(COM2+COM1);                         // Rs232初始化    
   ADS1222Init();                           // ADS1222初始化
 
  static hptime_t nowTime;
  
  //nowTime = ms_timestr2hptime("2000/12/22 10:12:22.000000"); 
  //nowTime = ms_timestr2hptime("2000/12/22 10:12:22.000000"); 
    nowTime = 0;    
  while(1){
           __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, enable interrupts 此處中斷大約100Hz
   
           
    // P1OUT |=BIT6;
    
     nowTime = 0;    
     nowTime  =   sysTime+CloCk_10KHz*100;   //get_hp_Gps_time(COM1_BUFFER);       
    
     for(int d1elAy=0;d1elAy<1600;d1elAy++);    //--delay 
             
                if(First_Data < 5){ First_Data += ADS1222_SELF_CALIBRATION();}
                else{ ADS1222_STANDBY_MODE(); }
                
     //        nowTime  =   get_hp_Gps_time(COM1_BUFFER);    
                
     string[0] = '$';                               //資料起始
     string[1] = (nowTime>>56)&0xff; 
     string[2] = (nowTime>>48)&0xff; 
     string[3] = (nowTime>>40)&0xff; 
     string[4] = (nowTime>>32)&0xff; 
     string[5] = (nowTime>>24)&0xff; 
     string[6] = (nowTime>>16)&0xff; 
     string[7] = (nowTime>>8)&0xff; 
     string[8] =  nowTime & 0xff ;
     string[9] =  (DATA[0]   >>  16 ) & 0xff ;
     string[10] =  (DATA[0]   >>  8 ) & 0xff;
     string[11] =  (DATA[0]    ) & 0xff;
     
  /*
     textpoint++;
     string[12] = ( textpoint >> 16)& 0xff;
     string[13] = ( textpoint >> 8)& 0xff;
     string[14] = ( textpoint )& 0xff;     
     if(textpoint>200)textpoint=0; 
*/

     string[12] =  (DATA[1]   >>  16 ) & 0xff ;
     string[13] =  (DATA[1]   >>  8 ) & 0xff;
     string[14] =  (DATA[1]    ) & 0xff;
     string[15] =  (DATA[2]   >>  16 ) & 0xff ;
     string[16] =  (DATA[2]   >>  8 ) & 0xff;
     string[17] =  (DATA[2]    ) & 0xff;
     string[18] =  '*';
     
     
      out_checksum = 0;
            for(int c=1;c<18;c++){
              out_checksum ^= string[c];
            }
     
     string[19] =  out_checksum;
     string[20] =  '\n';  
     
     
     //nTX1_Len = 16;
     
      
      RS232_Send_Char(string,21,COM2);   //---~ 1ms
      memset(string,0,30+1);
      
               if(sysTime_flag == 1){
                   sysTime_Buffer = get_hp_Gps_time(COM1_BUFFER);
                   sysTime_flag = 2;
                }   
               
               
    // P1OUT &= ~BIT6;               
  }
  //-------------------------------- start record-----------------------------
  
 
  
//   __no_operation();                          // For debugger                   
}
//----------------------------------------------------------------------------

#pragma vector=PORT1_VECTOR
__interrupt void P1ISR (void)
{
   //  P1OUT ^= BIT6;    //-----for check pps 
     
     if(sysTime_flag == 2){
        
        sysTime = sysTime_Buffer+1000000;
        sysTime_flag = 0;
        CloCk_10KHz = 0;
     }
     
     
     
     P1IFG &= ~BIT7;                         // 中斷旗號暫存器(0 無中斷 1 中斷等待處理)
}
//----------------------------------------------------------------------------
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
  CloCk_10KHz++;
  if(CloCk_10KHz>10001){      //-----保險@@~
    CloCk_10KHz=0;
     sysTime += 1000000;
    //P1OUT ^= BIT7;
  } 
  
}
//------------------------------------------------------------------------------


void Crystal_Init(){
    //--啟動 20MHZ 與32767HZ
    P5SEL |= 0x0C;                             // Port select XT2
    P7SEL |= 0x03;                             // Port select XT1
    UCSCTL6 &= ~(XT1OFF + XT2OFF);            // Set XT1 & XT2 On
    UCSCTL6 |= XCAP_3;                        // Internal load cap

    do    // Loop until XT1,XT2 & DCO stabilizes
    {
      UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + XT1HFOFFG + DCOFFG);
                                              // Clear XT2,XT1,DCO fault flags
      SFRIFG1 &= ~OFIFG;                      // Clear fault flags
    }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag

    UCSCTL6 &= ~XT2DRIVE0;                    // Decrease XT2 Drive according to expected frequency
    UCSCTL4 |= SELA_0 + SELS_5 + SELM_5 ;      // Select SMCLK, ACLK source and DCO source

  P4SEL = 0x02;                               // P4 option select 
  P4DIR = 0x02;                               // P4 outputs  1.6666MHz.......
 
//  P1DIR |=BIT6;
//  P1SEL |= BIT6;                            //-  設定P1.6為 SMCLK 輸出
    
  TBCCR0 = 5;                                 // PWM Period
  TBCCTL1 = OUTMOD_4;                         // toggle
  TBCCR1 = 2;                                 // CCR1 PWM Duty Cycle	
  
  TBCTL = TBSSEL_2 + MC_1 + TBCLR;            // SMCLK, upmode, clear TBR 
  
   TA0CCTL0 = CCIE;                          // CCR0 interrupt enabled
   TA0CCR0 = 2000-1;
   TA0CTL = TASSEL_2 + MC_1 + TACLR ;         // SMCLK, contmode, clear TAR
}
//--------------------------------------------------------------------------------
void ADS1222Init()
{
                            //------------P2------------------------------------
                            // P2.0(O) P2.1(I) P2.2(I) P2.3(I) P2.4(I) P2.5(I) P2.6(I) P2.7(I)
                            //   ↓      ↑      ↑      ↑      ↑      ↑      ↑      ↑
                            //  SCLK    CH1     CH2     CH3     CH4     CH5     CH6     CH7
                            //        interrupt
                            //------------P2------------------------------------
  P2SEL = 0;
  P2DIR |= BIT0;                                   //set P2.0 to output direction
  P2DIR &= ~(BIT1+BIT2+BIT3+BIT4);  //set P2.1~2.7 to get data on channel N
  P2OUT &= ~(BIT0);                                //set P2.0 to SCLK   (low)+
  P2IE  = 0 ;                                      //
  P2IES =0;                                        //
  P2IFG =0;                                        //
  P2IES |= (BIT1+BIT2+BIT3+BIT4);                         //選擇中斷模式(1 負緣觸發 )
  P2IE  |= (BIT1+BIT2+BIT3+BIT4);                         // Set P2.1 interrupt   ~加入判斷全部通道是否準備好資料
  
  for(long int i=0;i<159600;i++){}

  _EINT();  //啟動全域中斷
}
//------------------------------------------------------------------------------
//-------------以下這段  請依據示波器  對儀器做最佳的調整-----------------------
#pragma vector=PORT2_VECTOR
__interrupt void P2ISR (void)
{
  if((P2IFG & BIT1) ==BIT1)          {    DRDY_ready |= BIT1;P2IFG &= ~(BIT1); }
  if((P2IFG & BIT2) ==BIT2)          {    DRDY_ready |= BIT2;P2IFG &= ~(BIT2); }
  if((P2IFG & BIT3) ==BIT3)          {    DRDY_ready |= BIT3;P2IFG &= ~(BIT3); }
  if((P2IFG & BIT4) ==BIT4)          {    DRDY_ready |= BIT4;P2IFG &= ~(BIT4); }

  if(DRDY_ready != (BIT3+BIT2+BIT1+BIT4)) {  
     return; 
  } 

  DRDY_ready = 0;
  GPS_count++;
  if(GPS_count>=200)GPS_count=0;
  
                P2IFG &= ~(BIT3+BIT2+BIT1+BIT4);                   // 中斷旗號暫存器(0 無中斷 1 中斷等待處理)         
                __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
    
}
//------------------------------------------------------------------------------
int ADS1222_SELF_CALIBRATION()
{
int scLk,ch_n,delAy;
long delay_for_calibration;
  for(scLk=1;scLk<=24;scLk++){
    P2OUT |= (BIT0) ;                  //--同步要求輸出資料(SCLK)-----------上源觸發
    for(delAy=0;delAy<20;delAy++);                //DELAY

       for(ch_n=0; ch_n<4; ch_n++){
           if((P2IN&BITN[ch_n+1])==BITN[ch_n+1]){ DATA[ch_n] =( DATA[ch_n]<< 1) | 1;}
           else{ DATA[ch_n] =( DATA[ch_n]<< 1) | 0;}
       }

    P2OUT &= ~(BIT0) ;                  //--輸出資料結束--------------通知AD資料收集結束
    for(delAy=0;delAy<30;delAy++);
  }
    P2OUT |= (BIT0)  ;                  //--25th SCLK UP
    for(delAy=0;delAy<30;delAy++);    
    P2OUT &= ~(BIT0) ;                  //--25th SCLK DOWN
    for(delAy=0;delAy<30;delAy++);

    
    P2OUT |= (BIT0)  ;                  //--26th SCLK UP
    for(delAy=0;delAy<30;delAy++);    
    P2OUT &= ~(BIT0) ;                  //--26th SCLK DOWN
    for(delAy=0;delAy<30;delAy++);
 
    
    
    P2OUT |= (BIT0)  ;                  //--26th SCLK UP
    for(delay_for_calibration=0;delay_for_calibration<76000;delay_for_calibration++);    //--SELF CALIBRATION & STANDBY MODE    
    P2OUT &= ~(BIT0) ;
    
    return(1);
}
//------------------------------------------------------------------------------
void ADS1222_STANDBY_MODE(){
int scLk,ch_n,delAy;

  for(scLk=1;scLk<=24;scLk++){
    P2OUT |= (BIT0) ;                  //--同步要求輸出資料(SCLK)-----------上源觸發
    for(delAy=0;delAy<3;delAy++);      //DELAY  3*0.25us = 0.75us >> 100ns((P.14- ads1222)
    
       for(ch_n=0; ch_n<4; ch_n++){
           if((P2IN&BITN[ch_n+1])==BITN[ch_n+1]){ DATA[ch_n] =( DATA[ch_n]<< 1) | 1;}
           else{ DATA[ch_n] =( DATA[ch_n]<< 1) | 0;}
       }

   if(scLk!=24){
    P2OUT &= ~(BIT0) ;                  //--輸出資料結束--------------通知AD資料收集結束
    for(delAy=0;delAy<25;delAy++);
   }
  }

   for(ch_n=0; ch_n<4; ch_n++){
     if( (DATA[ch_n]&0x00FFFFFF) <= 0x007FFFFF){      //----2010 / 08/17 修改AD輸出格式 讓輸出0點附近連續
         DATA[ch_n] += 0x00800000; 
     }
     else{
         DATA[ch_n] -= 0x00800000;
     }
   }
 
  //if(1){
   for(delAy=0;delAy<1500;delAy++);    //--STANDBY MODE
   P2OUT &= ~(BIT0) ;
 // }
       
}
//------------------------------------------------------------------------------

void delay_ms(int del){
int dt;
  for(int delAy=0;delAy<del;delAy++)    //--delay 
    for(dt=0;dt<4000;dt++);
}
//----------------------------------------------------------------------------
void UART_Init(char com){
   //須要先啟動 XT2 16MHZ
  if( (com&COM1) == COM1 ){
          
      P3SEL = 0x30;                             // P3.4,5 = USCI_A0 TXD/RXD
      UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**

      UCA0CTL1 |= UCSSEL_1;                     // CLK = ACLK = 32767
      UCA0BR0 = 0x03;                           //   (see User's Guide)  9600
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
        UCA1BR1 = 0x00;


      UCA1MCTL = UCBRS_5+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0

      UCA1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
//      UCA1IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
  }

  UART_COM1_RX_count = 0;
  UART_COM2_RX_count = 0;

  
}
//----------------------------------------------------------------------------
char checksum = 0;
#pragma vector=USCI_A0_VECTOR  //COM1
__interrupt void USCI_A0_ISR(void)
{
  switch(__even_in_range(UCA0IV,4))
  {
    case 2:                                   // Vector 2 - RXIFG
      COM1_REC_BUFFER[UART_COM1_RX_count] = UCA0RXBUF;
        UART_COM1_RX_count++;
    
       if(COM1_REC_BUFFER[UART_COM1_RX_count-1] == '\n')
       {
         if(strncmp("$GPRMC",COM1_REC_BUFFER,6)==0){
            memcpy(COM1_BUFFER,COM1_REC_BUFFER,UART_COM1_RX_count+1);
            
            checksum = 0;
            for(int k=1;k<UART_COM1_RX_count-5;k++){
              checksum ^= COM1_BUFFER[k];
            }
            
            if(checksum == ((COM1_BUFFER[UART_COM1_RX_count-4]-48)*16+(COM1_BUFFER[UART_COM1_RX_count-3]-48))){
            
              checksum = 0;
              //-- 收到正確GPS字串~~ ~
              sysTime_flag = 1;  
              
              
            }
            
         }
            memset(COM1_REC_BUFFER,0,UART_COM1_RX_count+1);
            UART_COM1_RX_count = 0;
       }
    
    if(UART_COM1_RX_count>=128)UART_COM1_RX_count=0;   
      
  break;
  default: break;  
  }
  //  __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
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
  case 2:                                   // Vector 2 - RXIFG
//    while (!(UCA1IFG&UCTXIFG));           // USCI_A1 TX buffer ready?
//    UCA1TXBUF = UCA1RXBUF;                // TX -> RXed character
    UART_COM2_RX_BUF[UART_COM2_RX_count] = UCA1RXBUF;
    UART_COM2_RX_count++;
    if(UART_COM2_RX_count>=128)UART_COM2_RX_count=0;   
     
    break;
  default: break;  
  }
 //   __bic_SR_register_on_exit(LPM3_bits); // Exit LPM0
}
//----------------------------------------------------------------------------
void UART_SendByte(unsigned char data,char com)
{
  if((com&COM1) == COM1){
    while (!(UCA0IFG&UCTXIFG));             // USCI_A1 TX buffer ready?
    UCA0TXBUF = data;
  }
  if((com&COM2) == COM2){
    while (!(UCA1IFG&UCTXIFG));             // USCI_A1 TX buffer ready?
    UCA1TXBUF = data;
  }
  
}
void UART_SendStr(char *str,char com)
{
        while (*str != '\0')
        {
            UART_SendByte(*str++,com);	          // 發送數據
        }
        
}
void RS232_Send_Char(char *str,int count,char com)
{
        for(int i=0;i<count;i++)
        {
            UART_SendByte(*str++,com);	          // 發送數據
        }
}

//-----------------------------------------------------------------------------
void findStrPoint(char *a,char *ans,char feature,int n){
      int strcount = 0, Ncount = 0, pop = 0;
      while(a[strcount]!='\0')
      {
         if(a[strcount]== feature){
            pop++;
            if(pop==n)
            {
             ans[Ncount]='\0';
             return;
            }
            Ncount = 0;
         }
         else{
          ans[Ncount] = a[strcount];
          Ncount++;
         }
          strcount++;
      }
        ans[Ncount]='\0';
}