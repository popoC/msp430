//******************************************************************************
// 2012 0706 ---- 將最低時間單位改為 0.1ms / 10k
//******************************************************************************
//
//   interrupt        P1.5為接收訊號腳  中斷計時CLOCK
//   input   P1.1為接收命令  準備送資料                1 //準備丟
//   input   p1.2為檢查是否可發送資料腳                1 //可丟資料
//   output  p1.3為資料送出腳
//   output  p1.4 SCLK
//   output  p1.0 seascan 1pps output
//   output  p1.6 synchronize pin to seascan
//   input   p1.7接收資料 trigger
//   input   p2.6接收GPS之1pps訊號
//   資料格式 年  月  日  時  分  秒  毫秒
//            00  00  00  00  00  00  000    ///   7byte 每次傳送量
//
//------------------------------------------------------- 2009-2-3
//   interrupt        P1.5為接收訊號腳  中斷計時CLOCK
//   input   P1.1為接收命令  準備送資料                1 //準備丟
//   input   p1.2為檢查是否可發送資料腳                1 //可丟資料
//   output  p1.3為資料送出腳
//   output  p1.4 SCLK
//   output  p1.0 seascan 1pps output
//   output  p1.6 synchronize pin to seascan
//   input   p1.7接收資料 trigger
//   input   p2.6   接收  seascan 之1pps訊號
//   資料格式 年  月  日  時  分  秒  毫秒


//------------------------------------------------------- 2009-3-17
//-----2011/09/28韓國使用版本
#include  <msp430x20x3.h>
#include <string.h>
bool is_leapyear = false;   //判斷閏年


void InitTimeClock();
void IO_Send_Byte(int SendBuf);

void getTime_init();
void get_time();
int get_byte();

int delay ;
#define TimeOut 1500
//int ms,sec,minute,hour,day,month,year;
//int sMs ,sSec, sMinute , sHour, sDay, sMonth, sYear;
int time_str[7];
int time_str_buff[7];

//------------------------------------------------------------------------------
void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
_DINT();     // 關閉中斷

 InitTimeClock();
  P1DIR |=  (BIT0+BIT6+BIT3+BIT4);          // Set pin to output direction
  P1DIR &= ~(BIT1+BIT2+BIT7);               // Set pin to input direction
  P1OUT&=~BIT4;
  P1OUT&=~BIT6;                             //p1.6 一開始 為low

  P2SEL &= ~BIT6;                           //設定 P2.6為一般IO用途
  P2DIR &= ~BIT6;                           //set p2.6 input
  
  
  time_str[0] = 0;
  time_str[1] = 0;
  time_str[2] = 16;
  time_str[3] = 11;
  time_str[4] = 22;
  time_str[5] = 3;
  time_str[6] = 12;
  
// ms = 0;  sec = 0 ;   minute = 16 ;  hour = 11;  day = 22;  month = 3;  year = 12;
 delay = 0;
 /*
  CCTL0 = CCIE;                             // CCR0 interrupt enabled
  CCR0 = 102;
  TACTL = TASSEL_2 + MC_1;                  // SMCLK, upmode
 */
_EINT();    //開始中斷
   while(1){

     if( (P1IN&BIT1)==BIT1 )     //----SEACLOCK 送資料出去
     { 
     
       memcpy(time_str_buff,time_str,7);
       
      // for(int j=0;j<7;j++)time_str_buff[j]=time_str[j];
       //-----------------------開始送
        IO_Send_Byte(time_str_buff[6]);
        IO_Send_Byte(time_str_buff[5]);
        IO_Send_Byte(time_str_buff[4]);
        IO_Send_Byte(time_str_buff[3]);
        IO_Send_Byte(time_str_buff[2]);
        IO_Send_Byte(time_str_buff[1]);
        IO_Send_Byte(time_str_buff[0]);
     }
     if((P1IN&BIT7)==BIT7 )
     {
       _DINT();     // 關閉中斷
       getTime_init();
       get_time();   //  獲取GPS時間字串
        
       //----- 同步處理
       while((P2IN&BIT6)!=BIT6);    // 等待 SeaScan  之 1pps  由H變L
       while((P2IN&BIT6)==BIT6);    //因為 SeaScan 之 1pps已經與GPS同步過
       time_str[0] = -1;
       //--當GPS由L變H   開始數 直到SEASCAN的PluS到來
       //--新版本不需要由PIN19同步
       
       _EINT();    //開始中斷
     }


   }

}
/*
// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{
  P1OUT ^= 0x01;                            // Toggle P1.0
//  CCR0 += 50000;                            // Add Offset to CCR0
}
*/
//------------------------------------------------------------------------------
void IO_Send_Byte(int SendBuf){
   int SendN = 0;
   int time = 0;
     for(SendN=0;SendN<8;SendN++){
        P1OUT|=BIT4;
        while(((P1IN&BIT2)!=BIT2)&&(time < TimeOut) ){time++;}   //代表資料可以送
        time = 0;
       (SendBuf&0x01==0x01) ? P1OUT|=BIT3 : P1OUT&= ~BIT3;
        SendBuf >>=1 ;
        P1OUT&=~BIT4;
        while(((P1IN&BIT2)==BIT2)&&((time < TimeOut))){time++;}   //代表資料收走了
        time = 0;
     }
}
void InitTimeClock(){
  P1DIR &= ~BIT5;
  P1IE  = 0 ;                           //
  P1IES =0;                             //
  P1IFG =0;                             //
  P1IE  |= BIT5;                        // Set P1.5 interrupt
  P1IES &= ~BIT5;                        // 選擇中斷模式(0 由Lo變Hi觸發 )
}
//------------------------------------------------------------------------------
#pragma vector=PORT1_VECTOR
__interrupt void P1ISR (void)
{
     time_str[0]++;
 //    P1OUT &= ~BIT0; //-----for newboard v1.0

  if(time_str[0]>=125){
     time_str[0]=0;time_str[1]++;
 //    P1OUT |= BIT0;
  }//-----for newboard v1.0
   if(time_str[1]>=60){time_str[1]=0;time_str[2]++;}
   if(time_str[2]>=60){time_str[2]=0;time_str[3]++;}
   if(time_str[3]>=24){time_str[3]=0;time_str[4]++;}
   
  if(is_leapyear){
   if(time_str[4]>29){  if(time_str[5]==2 ){time_str[4]=1;time_str[5]++;}   }   
   if(time_str[4]>30){  if((time_str[5]==4)||(time_str[5]==6)||(time_str[5]==9)||(time_str[5]==11)){time_str[4]=1;time_str[5]++;}   }
   if(time_str[4]>31){  time_str[4]=1;time_str[5]++;   }
  }
  else{
   if(time_str[4]>28){  if(time_str[5]==2 ){time_str[4]=1;time_str[5]++;}   }   
   if(time_str[4]>30){  if((time_str[5]==4)||(time_str[5]==6)||(time_str[5]==9)||(time_str[5]==11)){time_str[4]=1;time_str[5]++;}   }
   if(time_str[4]>31){  time_str[4]=1;time_str[5]++;   }    
  }
   if(time_str[5]>12){time_str[5]=1;time_str[6]++;
         if( ((time_str[6]%4)==0) &&((time_str[6]%100)!=0) || (time_str[6]%400)==0){  //--//-閏年   2月有29天
                    is_leapyear = true;      }
         else{      is_leapyear = false;  } 
   }
     P1IFG &= ~BIT5;                         // 中斷旗號暫存器(0 無中斷 1 中斷等待處理)
}
//------------------------------------------------------------------------------
void getTime_init(){
  P1OUT &= ~(BIT4+BIT3);    //
}
void get_time(){
   P1OUT |= BIT3;
   time_str[6]  =  get_byte();
   P1OUT &= ~BIT3;
   time_str[5] =  get_byte();
   time_str[4]   =  get_byte();
   time_str[3]  =  get_byte();
   time_str[2]=  get_byte();
   time_str[1]   =  get_byte();
//   ms    =  get_byte();
delay = get_byte();
   time_str[0]    =  124;
   
   
  if( ((time_str[6]%4)==0) &&((time_str[6]%100)!=0) || (time_str[6]%400)==0){  //--//-閏年   2月有29天
                 is_leapyear = true;      }
  else{      is_leapyear = false;       }      
   
   
}

int get_byte(){
int buf=0;
int time = 0;
 for(int i=0;i<8;i++){
   while(((P1IN&BIT2)!=BIT2)&&(time < TimeOut)){time++;}
   time = 0;
   P1OUT |= BIT4;
   while(((P1IN&BIT2)==BIT2)&&(time < TimeOut)){time++;}
   time = 0;
   if(P1IN & BIT1) buf+=0x01 <<i;
   P1OUT &= ~BIT4;
 }
 return(buf);
}