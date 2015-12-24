//-- 20151224
// for OBS RTC
//--  com1 -> obs logger
//--  com2 -> gps 
//   command List
//--  P1.6 OBS-1pps
//--  P1.7 GPS-1pps
//---------------------------
#include  <msp430x54xA.h>
#include "stdio.h"

#define COM_BUF_Size 128

int Control_Mode = 1; //--- command is ##--*    (-- 為模式編號, 0~65535)
int Diff_1PPS = 0;

int CloCk_10KHz =0;
void DelayMs(int ms);
void findStrPoint(char *a,char *ans,char feature,int n);
char Getp[50];

void Open_Syn_Interrupt();
void UART_Init(BYTE com);

//----------for rs232-----------------------------------------------------------
char string[50];

char COM1_BUFFER[COM_BUF_Size];    // -- 115200 msp430
char COM2_BUFFER[COM_BUF_Size];    // -- 9600   GPS
char COM3_BUFFER[COM_BUF_Size];    // -- 9600   PC


char setTime[12];
void Delay1s(void);


bool wait_seascan_pps = false;
bool obs_time_str_come = false;
long int gPs_mms  = 0;
long int oBs_mms  = 0;


int Gps_yy;
int Gps_MM;
int Gps_dd;


int Gps_hh;
int Gps_mm;
int Gps_ss;


int Obs_hh;
int Obs_mm;
int Obs_ss;

float nEw_ms;


int GPS_MODE = 0;
//-----------------------------------------------------------------------------
void Crystal_Init();

void main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
 _DINT();     // 關閉中斷
 
   Crystal_Init();                          // 震盪器初始化
   UART_Init(COM1+COM2+COM3);                         // Rs232初始化    
 

 P1DIR = 0x01 + 0x20;

 Open_Syn_Interrupt();
 
   while(1){
  
     __bis_SR_register(GIE+LPM0_bits);

    
     
     if(cOm_fLag == pCt_tIme){
     
       sprintf(string,"mode = %05d  ",Control_Mode);
      
       Print_Memo(0,0,string);
     
      
       
       
     }
     
       
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
     
     if(wait_seascan_pps){
       wait_seascan_pps = false;
 
      //if(nEw_ms < 950)
        sprintf(string,"%08ld/%05.1f.",Diff_1PPS_2,nEw_ms);
 
      if(Control_Mode==1)Print_Memo(0,0,string);
      
        sprintf(string,"%08ld",Diff_1PPS_2);
    //  if(Control_Mode==2)UART_SendStr(
      
     
     }
     
     
     
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
  
     if(obs_time_str_come){
        oBs_mms+=1;
       
        Diff_1PPS_2 = (gPs_mms - oBs_mms)*10000 + Diff_1PPS;
       
        obs_time_str_come = false;
        wait_seascan_pps = true;
     } 
     
     
  }  
  if((P1IFG & BIT7) ==BIT7){  
    CloCk_10KHz = 0;
    gPs_mms = Gps_hh*1440+Gps_mm*60+Gps_ss+1; //  +1 是因為GPS字串傳出比1PPS慢  GPS
    
    
     if(Control_Mode == 5){
       Control_Mode = 1;
       if(Gps_ss < 58){
	setTime[0] = '@'; //
	setTime[1] = 11;   //
	setTime[2] = 't'; // 0x7A , need bigger than '0x60'
        setTime[3] = Gps_yy;
        setTime[4] = Gps_MM;
        setTime[5] = Gps_dd;
        setTime[6] = Gps_hh;
        setTime[7] = Gps_mm;
        setTime[8] = Gps_ss+2;
        setTime[9] = 0;
	setTime[10] = '$'; //
        
        for(int i=0;i<11;i++){
         UART_SendByte(setTime[i],COM1);
        }
       }
       
     }
    
    
    P1IFG &= ~BIT7;                      
  }  
  
//   __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0

}
//-------------------------------------------------------------------------
void int2str(long int i,char *s) {
     sprintf(s,"%09ld",i);
}
//-------------------------------------------------------------------------
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
     
  //  sprintf(string," counter  %03d   ",is);is++;if(is>200)is=0;
  //  Print_Memo(0,0,string); 
    
   
  }
}
//----------------------------------------------------------------------------
void Delay1s(void)
{
   for(int i=0;i<200;i++)Delay5Ms();
}
//----------------------------------------------------------------------------
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

//--------------------------------------

void UART_Init(BYTE com){
   //須要先啟動 XT2 20MHZ
  if( (com&COM1) == COM1 ){
          
      P3SEL = 0x30;                             // P3.4,5 = USCI_A0 TXD/RXD
      UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**

      UCA0CTL1 |= UCSSEL_2;                     // CLK = SCLK = 20MHZ
      UCA0BR0 = 0xAD;                           //   (see User's Guide)  20M/115200 = 173
      UCA0BR1 = 0x00;                           //

      UCA0MCTL = UCBRS_5+UCBRF_0;              // user guide p954 Rev. O

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

      UCA2CTL1 |= UCSSEL_1;                     // CLK = ACLK
      UCA2BR0 = 0x0D;                           // (see User's Guide) for seascan
      UCA2BR1 = 0x00;                           //
      UCA2MCTL = UCBRS_6+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0

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
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector 2 - RXIFG
    
     //if(COM1_to_COM4)

    if(Control_Mode==3){
        while (!(UCA3IFG&UCTXIFG));        // com1 to com4
        UCA3TXBUF = UCA0RXBUF;
    }else{
    
    
        UART_COM1_RX_BUF[UART_COM1_RX_count] = UCA0RXBUF;
        UART_COM1_RX_count++;
        
         //  if(UART_COM1_RX_BUF[UART_COM1_RX_count-1] == '=')
        if((UART_COM1_RX_BUF[0] == 't'))   
        {
          if(UART_COM1_RX_count>=11){
            memcpy(COM1_Command,UART_COM1_RX_BUF,UART_COM1_RX_count+1);
            UART_COM1_RX_count = 0;
            cOm_fLag = oBs_tIme;
            __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
          }
        }
        else{
           UART_COM1_RX_count = 0;
        }
          
        
        
      if(UART_COM1_RX_count>=COM_BUF_Size)UART_COM1_RX_count=0;
    }
    break;
  case 4:break;                             // Vector 4 - TXIFG
  default: break;  
  }
}
//------------------------------------------------------------------------------
#pragma vector=USCI_A1_VECTOR //COM2
__interrupt void USCI_A1_ISR(void)
{
  
  switch(__even_in_range(UCA1IV,4))
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector 2 - RXIFG
//    while (!(UCA1IFG&UCTXIFG));           // USCI_A1 TX buffer ready?
//    UCA1TXBUF = UCA1RXBUF;                // TX -> RXed character
    UART_COM2_RX_BUF[UART_COM2_RX_count] = UCA1RXBUF;
    UART_COM2_RX_count++;
    
       if(UART_COM2_RX_BUF[UART_COM2_RX_count-1] == '\n')
       {
            memcpy(COM2_Command,UART_COM2_RX_BUF,UART_COM2_RX_count+1);
            UART_COM2_RX_count = 0;
            cOm_fLag = gPs_tIme;
            __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
       
       }
 
    
    
    
    
    
    if(UART_COM2_RX_count>=COM_BUF_Size)UART_COM2_RX_count=0;   
    
      
    break;
  case 4:break;                             // Vector 4 - TXIFG
  default: break;  
  }
 //   __bic_SR_register_on_exit(LPM3_bits); // Exit LPM0
}
//------------------------------------------------------------------------------