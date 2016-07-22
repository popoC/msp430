//-- 20151224
// for OBS RTC
//--  COM1 115200 -> pc
//--  COM2 9600   -> gps 
//--  COM3 115200   -> obs logger

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

#define GPS_PPS_PIN BIT7
#define OBS_PPS_PIN BIT6


int Control_Mode = 2;  //-    0. 時間比對
                       //-    1. 切換AD輸出
                       //-    2. 輸出OBS時間


char GPS_Time_String[]  = "2000/01/01 00:00:00.000000";
char diff_Time_String[] = "18446744073709551616           ";

char OBS_Time_String[] = "2000/01/01 00:00:00.000000";
float OBS_ms = 0;
int OBS_Time_Flag = 0;
int OBS_Time_PPS_Flag = 0;

int OBS_10k_Buffer = 0;



static hptime_t OBS_Time_hp,diff_Time_hp;       //--   1s = 1000000 counter

hptime_t CCR1_hp;
float ccr1_counter = 0;


static int  GPSTime_flag = 0;
static hptime_t GPSTime,GPSTime_Buffer;       //--   1s = 1000000 counter


char checksum = 0;


void findStrPoint(char *a,char *ans,char feature,int n);

int Diff_1PPS = 0;

hptime_t CloCk_10KHz =0;
void DelayMs(int ms);


void Open_Syn_Interrupt();
void UART_Init(int com);

time_t GPS_time;

//----------for rs232-----------------------------------------------------------
char string[50];

char COM1_BUFFER[COM_BUF_Size];    // -- 115200 msp430
char COM1_REC_BUFFER[COM_BUF_Size];    // -- receive buffer

char COM2_BUFFER[COM_BUF_Size];    // -- 9600   GPS
char COM2_REC_BUFFER[COM_BUF_Size];    // -- receive buffer

char COM3_BUFFER[COM_BUF_Size];    // -- 115200   PC
char COM3_REC_BUFFER[COM_BUF_Size];    // -- receive buffer

int UART_COM1_RX_count,UART_COM2_RX_count,UART_COM3_RX_count;
void UART_SendByte(unsigned char data,char com);
void UART_SendStr(char *str,char com);
void RS232_Send_Char(char *str,int count,char com);

//------------------------------------------------------------------------------



//-----------------------------------------------------------------------------
void Crystal_Init();

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

void main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
// _DINT();     // 關閉中斷

 Crystal_Init();                          // 震盪器初始化
 UART_Init(COM1+COM2+COM3);                         // Rs232初始化    
//  UART_Init(COM1+COM3);                         // Rs232初始化    

 P1DIR = 0x01 + 0x20;

 Open_Syn_Interrupt();
 
 
  _EINT();
  __bis_SR_register(GIE);
   while(1){
  
   //  __bis_SR_register(GIE+LPM0_bits);
     
     if(Control_Mode == 0){
     
          if(OBS_Time_Flag){
              OBS_Time_PPS_Flag = 1;
              OBS_Time_Flag = 0;

                OBS_Time_String[2] = COM3_BUFFER[1]/10+48;             //char OBS_Time_String[] = "2000/12/22 10:12:22.000000";
                OBS_Time_String[3] = COM3_BUFFER[1]%10+48;  

                OBS_Time_String[5] = COM3_BUFFER[2]/10+48;
                OBS_Time_String[6] = COM3_BUFFER[2]%10+48;  

                OBS_Time_String[8] = COM3_BUFFER[3]/10+48;
                OBS_Time_String[9] = COM3_BUFFER[3]%10+48;  
              
                OBS_Time_String[11] = COM3_BUFFER[4]/10+48;
                OBS_Time_String[12] = COM3_BUFFER[4]%10+48;  

                OBS_Time_String[14] = COM3_BUFFER[5]/10+48;
                OBS_Time_String[15] = COM3_BUFFER[5]%10+48;  

                OBS_Time_String[17] = COM3_BUFFER[6]/10+48;
                OBS_Time_String[18] = COM3_BUFFER[6]%10+48;  
                
              OBS_Time_hp = ms_timestr2hptime(OBS_Time_String); 
           
          
              while(OBS_Time_PPS_Flag);
           
              OBS_Time_hp = OBS_Time_hp+1000000;  //OBS_ms = CloCk_10KHz;
              ccr1_counter = ccr1_counter*30.517578125;
              CCR1_hp =  (hptime_t)ccr1_counter;
              diff_Time_hp  =   (GPSTime+(CCR1_hp)) - OBS_Time_hp;
      //    ms_hptime2mdtimestr(diff_Time_hp, diff_Time_String, 1);   //   ms_hptime2mdtimestr(sysTime, diff_Time_String, 1);
              diff_Time_String[0] = '%';
              int rec = snprintf(&diff_Time_String[1],30,"%lld",diff_Time_hp);
              diff_Time_String[rec+1] = '\r';          diff_Time_String[rec+2] = '\n';          diff_Time_String[rec+3] = 0;
              UART_SendStr(diff_Time_String,COM1); 
         
          }     
          if(GPSTime_flag == 1){
                   GPSTime_Buffer = get_hp_Gps_time(COM2_BUFFER);
                   GPSTime_flag = 2;
                   GPS_Time_String[21]='\r';                   GPS_Time_String[22]='\n';                   GPS_Time_String[23]=0;
                   //  UART_SendStr(GPS_Time_String,COM3);
                   
          }
     }
     else if(Control_Mode==1){
     
     
     
     }
     else if(Control_Mode==2){
     
     
     
     }
     
     
   }
   
}

//-------------------------------------------------------------------------
void Open_Syn_Interrupt(){
  P1IE  |= OBS_PPS_PIN+GPS_PPS_PIN;                   // Set P1.1, 1.2, 1.5interrupt
  P1IES |=  OBS_PPS_PIN;                       // 選擇中斷模式(1 負緣觸發 ) for Seascan  
  P1IES &= ~GPS_PPS_PIN;                       // 選擇中斷模式(0 正緣觸發 ) 由負變正 for GPS
  P1IFG &= ~(OBS_PPS_PIN+GPS_PPS_PIN);                             //
}
//-------------------------------------------------------------------------
#pragma vector=PORT1_VECTOR
__interrupt void P1ISR (void)
{
  if((P1IFG & OBS_PPS_PIN) ==OBS_PPS_PIN){
     //Diff_1PPS = CloCk_10KHz;
     
     ccr1_counter = TA0R;
     OBS_Time_PPS_Flag = 0;
    P1IFG &= ~OBS_PPS_PIN;         
                        
  }  
  if((P1IFG & GPS_PPS_PIN) ==GPS_PPS_PIN){
     if(GPSTime_flag == 2){

        TA0CTL |=  TACLR  ;  //計數歸零
        CloCk_10KHz = 0;
        GPSTime = GPSTime_Buffer+1000000;
        GPSTime_flag = 0;
        
        
        
     }
      P1IFG &= ~GPS_PPS_PIN; 
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
    //TA0CCR0 = 1200-1;                         // 12MHz -> 10Khz
    TA0CCR0 = 32768-1;                         // 32768 -> 10Khz
    
    
    TA0CTL = TASSEL_1 + MC_1 + TACLR  ;         // SMCLK, contmode, clear TAR
 
  
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
  /*
  CloCk_10KHz++;
  if(CloCk_10KHz>10010){
    CloCk_10KHz=0;
     sysTime += 1000000;
    //P1OUT ^= 0x20;
    
  } P1OUT ^= 0x20; 
  */
    GPSTime += 1000000;
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
      
    //  UCA0CTL0 = 0;
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

      //UCA2CTL0 = 0;
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
    
       if(UART_COM1_RX_count>=COM_BUF_Size)UART_COM1_RX_count=0;     
        
       if( (COM1_REC_BUFFER[0]=='@')){        //- 命令拆解
       if((UART_COM1_RX_count>0)&&(UART_COM1_RX_count==COM1_REC_BUFFER[1])&&(COM1_REC_BUFFER[UART_COM1_RX_count-1]=='$')){

         switch(COM1_REC_BUFFER[2]){
         case 'T':
           Control_Mode = 2;
           
           RS232_Send_Char(COM1_REC_BUFFER,UART_COM1_RX_count,COM3);
           
           break;
         case 'A':
           Control_Mode = 1;
           
           RS232_Send_Char(COM1_REC_BUFFER,UART_COM1_RX_count,COM3);
           
           break;
         case 'S':
           Control_Mode = 2;           
           
           RS232_Send_Char(COM1_REC_BUFFER,UART_COM1_RX_count,COM3);
          //  StartRecord 
           break;
         case 'D':
           Control_Mode = 0;           
         
           break;
           
         }
       UART_COM1_RX_count = 0;  
       }
     }
     else{
       UART_COM1_RX_count = 0;
     }

     
     
    if(UART_COM1_RX_count>0){
     if(COM1_REC_BUFFER[UART_COM1_RX_count-1]=='$')UART_COM1_RX_count = 0;
    }
  
        
       
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
         if(strncmp("$GPRMC",COM2_REC_BUFFER,6)==0){
            memcpy(COM2_BUFFER,COM2_REC_BUFFER,UART_COM2_RX_count+1);
            
            checksum = 0;
            for(int k=1;k<UART_COM2_RX_count-5;k++){
              checksum ^= COM2_BUFFER[k];
            }
            
            if(checksum == ((COM2_BUFFER[UART_COM2_RX_count-4]-48)*16+(COM2_BUFFER[UART_COM2_RX_count-3]-48))){
            
              checksum = 0;
              //-- 收到正確GPS字串~~ ~
              GPSTime_flag = 1;  
              
              
            }
            
         }
            memset(COM2_REC_BUFFER,0,UART_COM2_RX_count+1);
            UART_COM2_RX_count = 0;
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
    
        if(Control_Mode==0){   
        
            if(COM3_REC_BUFFER[UART_COM3_RX_count-1] == 1&&(UART_COM3_RX_count==11))
            { 
                  OBS_10k_Buffer = CloCk_10KHz;
            
                  memcpy(COM3_BUFFER,COM3_REC_BUFFER,UART_COM3_RX_count+1);
            
            
                  OBS_Time_Flag = 1;
            
                  memset(COM3_REC_BUFFER,0,UART_COM3_RX_count+1);
                  UART_COM3_RX_count = 0;
            
                  __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
       
            }
            else if(COM3_REC_BUFFER[UART_COM3_RX_count-1] == 1&&(UART_COM3_RX_count>11)){
                  UART_COM3_RX_count = 0;
            }
        }
        else{
        
          UART_SendByte(COM3_REC_BUFFER[UART_COM3_RX_count-1],COM1);
          UART_COM3_RX_count--;
        
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
