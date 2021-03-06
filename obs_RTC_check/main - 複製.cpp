//-- 20151224
// for OBS RTC
//--  COM1 115200 -> obs logger
//--  COM2 9600   -> gps 
//--  COM3 115200   -> pc

//   command List
//--  P1.6 OBS-1pps
//--  P1.7 GPS-1pps
//---------------------------
#include  <msp430x54xA.h>
#include "stdio.h"
#include "string.h"
#include "time.h"
#include "hp_time.h"

#define COM_BUF_Size 128
#define COM1 0x01
#define COM2 0x02
#define COM3 0x04

#define gPs_tIme 1

#define BUF_SIZE 10
int cOm_fLag = 0;


typedef struct gps_info
{
char utc_time[BUF_SIZE];
char status[2];
char latitude_value[BUF_SIZE];
char latitude;
char longtitude_value[BUF_SIZE];
char longtitude;
char speed[BUF_SIZE];
char azimuth_angle[BUF_SIZE];
char utc_data[BUF_SIZE];
};//GPS_INFO;
char *gpsStr;
//GPS_INFO gPsData;
void findStrPoint(char *a,char *ans,char feature,int n);
int Control_Mode = 1; //--- command is ##--*    (-- 為模式編號, 0~65535)
int Diff_1PPS = 0;

int CloCk_10KHz =0;
void DelayMs(int ms);

char Getp[50];

void Open_Syn_Interrupt();
void UART_Init(int com);

time_t GPS_time;

//----------for rs232-----------------------------------------------------------
char string[50];

char COM1_BUFFER[COM_BUF_Size];    // -- 115200 msp430
char COM1_REC_BUFFER[COM_BUF_Size];    // -- receive buffer

char COM2_BUFFER[COM_BUF_Size];    // -- 9600   GPS
char COM2_REC_BUFFER[COM_BUF_Size];    // -- receive buffer

char COM3_BUFFER[COM_BUF_Size];    // -- 9600   PC
char COM3_REC_BUFFER[COM_BUF_Size];    // -- receive buffer

int UART_COM1_RX_count,UART_COM2_RX_count,UART_COM3_RX_count;

void UART_SendStr(char *str,char com);
void RS232_Send_Char(char *str,int count,char com);

//------------------------------------------------------------------------------


float nEw_ms;

int GPS_MODE = 0;
//-----------------------------------------------------------------------------
void Crystal_Init();

void main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
 _DINT();     // 關閉中斷
 
//char time1[] = 
static hptime_t s1,s2,s3,s4,s5;

s1 = ms_timestr2hptime("1980/12/22 10:12:22.016803");
s2 = ms_timestr2hptime("1980/12/12 10:12:22.016800");
s3 = ms_timestr2hptime("2015/12/22 10:12:22.016898");
s4 = s3-s1;
s5 = s1-s2;

 Crystal_Init();                          // 震盪器初始化
   UART_Init(COM1+COM2+COM3);                         // Rs232初始化    
 
gps_info GPS_INFO;
 P1DIR = 0x01 + 0x20;

 Open_Syn_Interrupt();
 
   while(1){
  
     __bis_SR_register(GIE+LPM0_bits);
     if(cOm_fLag == gPs_tIme){ 
          cOm_fLag = 0;
          
          if( strncmp("$GPRMC",COM2_BUFFER,6)==0){ 
            
           findStrPoint(COM2_BUFFER,GPS_INFO.status ,',',3);
           if(GPS_INFO.status[0]=='A'); //--- 這裡開始數據有效
           findStrPoint(COM2_BUFFER,GPS_INFO.utc_time,',',2);
           findStrPoint(COM2_BUFFER,GPS_INFO.utc_data,',',10);
           
           //strcpy(GPS_INFO.utc_data , gpsStr);
              //  gpsStr = strtok(NULL,",");
              
            UART_SendStr("hih2i \r\n",COM3);  
              
           cOm_fLag = 0;
          }
     }
     /*
       
     if(cOm_fLag == oBs_tIme){
      
          nEw_ms =  ((float)COM1_BUFFER[7] / 125)*1000 + ((float)COM1_BUFFER[8]/10);


          if((nEw_ms > 100) &&(nEw_ms < 960)){
            if(obs_time_str_come == false){
       
              //P1OUT |= 0x20;
         
              obs_time_str_come = true;
  
              Obs_hh = COM1_BUFFER[4];
              Obs_mm = COM1_BUFFER[5];
              Obs_ss = COM1_BUFFER[6];
       
             oBs_mms = Obs_hh*1440 + Obs_mm*60 + Obs_ss; 
         
             DelayMs(20);
             //P1OUT &= ~0x20;
            }
          }
       
      memset( COM1_Command, 0, sizeof(COM1_Command) ); 
     }
     
     
     
     if(cOm_fLag == gPs_tIme){     // COM2 GPS收到 /n 執行
      
       if( strstr(COM2_Command,"GPRMC")!=0 ){
         
         //P1OUT |= 0x20;
         
                 findStrPoint(COM2_Command,Getp,',',3);
                  
                 GPS_MODE = Getp[0];
                 memset( string, 0, sizeof(string) ); 

                  
                  
                  
                  findStrPoint(COM2_Command,Getp,',',10);
                Gps_yy = (Getp[4]-48)*10+(Getp[5]-48);  
                Gps_MM = (Getp[2]-48)*10+(Getp[3]-48);  
                Gps_dd = (Getp[0]-48)*10+(Getp[1]-48);  

   
                  findStrPoint(COM2_Command,Getp,',',2);
               Gps_hh = (Getp[0]-48)*10+(Getp[1]-48);  
               Gps_mm = (Getp[2]-48)*10+(Getp[3]-48);  
               Gps_ss = (Getp[4]-48)*10+(Getp[5]-48);  

               if(GPS_MODE == 'A'){
                 sprintf(string,"A = %02d:%02d:%02d .",Gps_hh,Gps_mm,Gps_ss); 
               }
               else{
                 sprintf(string,"V = %02d:%02d:%02d .",Gps_hh,Gps_mm,Gps_ss); 
               }
         Print_Memo(0,1,string);
         memset( COM2_Command, 0, sizeof(COM2_Command) );
       //           P1OUT &= ~0x20;

       }
     }
     
  */
     
     
   }
   
}

//-------------------------------------------------------------------------
void Open_Syn_Interrupt(){
  P1IE  |= BIT6+BIT7;                   // Set P1.1, 1.2, 1.5interrupt
  P1IES |=  BIT6;                       // 選擇中斷模式(1 負緣觸發 ) for Seascan  
  P1IES &= ~BIT7;                       // 選擇中斷模式(0 正緣觸發 ) 由負變正 for GPS
  P1IFG &= ~(BIT6+BIT7);                             //
}
//-------------------------------------------------------------------------
#pragma vector=PORT1_VECTOR
__interrupt void P1ISR (void)
{
  if((P1IFG & BIT6) ==BIT6){
     Diff_1PPS = CloCk_10KHz;
    P1IFG &= ~BIT6;         
 
    P1IFG &= ~BIT7;                      
  }  
  


}
//-------------------------------------------------------------------------
void int2str(long int i,char *s) {
     sprintf(s,"%09ld",i);
}
//-------------------------------------------------------------------------
void Crystal_Init(){
    //--啟動 12MHZ 與32767HZ
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
    UCSCTL4 |= SELA_0 + SELS_5 + SELM_5;      // Select SMCLK, ACLK source and DCO source
 
    TA0CCTL0 = CCIE;                          // CCR0 interrupt enabled
    TA0CCR0 = 2000-1;
    TA0CTL = TASSEL_2 + MC_1 + TACLR ;         // SMCLK, contmode, clear TAR
 
  
}
//--------------------------------------------------------------------------
void DelayMs(int ms){
// MCLK = 20Mhz
   
  for(int i=0;i<ms;i++)
  __delay_cycles(20000);
 
}
//--------------------------------------------------------------------------
// Timer A0 interrupt service routine

int is=9;
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
  CloCk_10KHz++;
  if(CloCk_10KHz>20000){
    CloCk_10KHz=0;
    P1OUT ^= 0x20;
     
  }
}
//----------------------------------------------------------------------------


//--------------------------------------

void UART_Init(int com){
   //須要先啟動 XT2 12MHZ
  if( (com&COM1) == COM1 ){
          
      P3SEL = 0x30;                             // P3.4,5 = USCI_A0 TXD/RXD
      UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**

      UCA0CTL1 |= UCSSEL_2;                     // CLK = SCLK = 12MHZ
      UCA0BR0 = 0x68;                           //   (see User's Guide)  12M/115200 = 104
      UCA0BR1 = 0x00;                           //

      UCA0MCTL = UCBRS_1+UCBRF_0;              // user guide p954 Rev. O

      UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
      UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
  }
  if( (com&COM2) == COM2 ){

      P5SEL |= 0xc0;                             // P5.6,7 = USCI_A0 TXD/RXD
      UCA1CTL1 |= UCSWRST;                      // **Put state machine in reset**

      UCA1CTL1 |= UCSSEL_1;                     // CLK = ACLK
      UCA1BR0 = 0x03; 
      UCA1BR1 = 0x00;                           // 32767 / 9600 = 6
      UCA1MCTL = UCBRS_3+UCBRF_0;               // user guide p909

      UCA1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
      UCA1IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
  }

   if( (com&COM3) == COM3 ){

      P9SEL |= 0x30;                             // P9.4,5 = USCI_A0 TXD/RXD
      UCA2CTL1 |= UCSWRST;                      // **Put state machine in reset**

      UCA2CTL1 |= UCSSEL_2;                     // CLK = SCLK
      UCA2BR0 = 0x68;                           // (see User's Guide) 115200
      UCA2BR1 = 0x00;                           //
      UCA2MCTL = UCBRS_1+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0

      UCA2CTL0 = 0;
      UCA2CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
      UCA2IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
  }
 
  UART_COM1_RX_count = 0;
  UART_COM2_RX_count = 0;
  UART_COM3_RX_count = 0;
  
}
//--------------------------------------------
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
            memcpy(COM1_BUFFER,COM1_REC_BUFFER,UART_COM1_RX_count+1);
            UART_COM1_RX_count = 0;
            __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
       
       }
 
    
    if(UART_COM1_RX_count>=COM_BUF_Size)UART_COM1_RX_count=0;   
       
     break;
  default: break;  
  }
}
//------------------------------------------------------------------------------
#pragma vector=USCI_A1_VECTOR //COM2
__interrupt void USCI_A1_ISR(void)
{
  
  switch(__even_in_range(UCA1IV,4))
  {
     case 2:                                   // Vector 2 - RXIFG
        COM2_REC_BUFFER[UART_COM2_RX_count] = UCA1RXBUF;
        UART_COM2_RX_count++;
    
       if(COM2_REC_BUFFER[UART_COM2_RX_count-1] == '\n')
       {
            memcpy(COM2_BUFFER,COM2_REC_BUFFER,UART_COM2_RX_count+1);
            memset(COM2_REC_BUFFER,0,UART_COM2_RX_count+1);
            UART_COM2_RX_count = 0;
            cOm_fLag = gPs_tIme;
            __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
       
       }
 
    
    if(UART_COM2_RX_count>=COM_BUF_Size)UART_COM2_RX_count=0;   
      
    break;
  default: break;  
  }
 //   __bic_SR_register_on_exit(LPM3_bits); // Exit LPM0
}
//------------------------------------------------------------------------------
#pragma vector=USCI_A2_VECTOR //COM3
__interrupt void USCI_A2_ISR(void)
{
  
  switch(__even_in_range(UCA2IV,4))
  {
     case 2:                                   // Vector 2 - RXIFG
        COM3_REC_BUFFER[UART_COM3_RX_count] = UCA2RXBUF;
        UART_COM3_RX_count++;
    
       if(COM3_REC_BUFFER[UART_COM3_RX_count-1] == '\n')
       {
            memcpy(COM3_BUFFER,COM3_REC_BUFFER,UART_COM3_RX_count+1);
            memset(COM3_REC_BUFFER,0,UART_COM3_RX_count+1);
            UART_COM3_RX_count = 0;
            __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
       
       }
    
    if(UART_COM3_RX_count>=COM_BUF_Size)UART_COM3_RX_count=0;   
      
    break;
  default: break;  
  }
 //   __bic_SR_register_on_exit(LPM3_bits); // Exit LPM0
}
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
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
  if((com&COM3) == COM3){
    while (!(UCA2IFG&UCTXIFG));             // USCI_A1 TX buffer ready?
    UCA2TXBUF = data;
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
