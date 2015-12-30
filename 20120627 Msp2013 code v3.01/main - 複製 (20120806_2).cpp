//******************************************************************************
// 2012 0706 ---- �N�̧C�ɶ����אּ 0.1ms / 10k
//******************************************************************************
//
//   interrupt        P1.5�������T���}  ���_�p��CLOCK
//   input   P1.1�������R�O  �ǳưe���                1 //�ǳƥ�
//   input   p1.2���ˬd�O�_�i�o�e��Ƹ}                1 //�i����
//   output  p1.3����ưe�X�}
//   output  p1.4 SCLK
//   output  p1.0 seascan 1pps output
//   output  p1.6 synchronize pin to seascan
//   input   p1.7������� trigger
//   input   p2.6����GPS��1pps�T��
//   ��Ʈ榡 �~  ��  ��  ��  ��  ��  �@��
//            00  00  00  00  00  00  000    ///   7byte �C���ǰe�q
//
//------------------------------------------------------- 2009-2-3
//   interrupt        P1.5�������T���}  ���_�p��CLOCK
//   input   P1.1�������R�O  �ǳưe���                1 //�ǳƥ�
//   input   p1.2���ˬd�O�_�i�o�e��Ƹ}                1 //�i����
//   output  p1.3����ưe�X�}
//   output  p1.4 SCLK
//   output  p1.0 seascan 1pps output
//   output  p1.6 synchronize pin to seascan
//   input   p1.7������� trigger
//   input   p2.6   ����  seascan ��1pps�T��
//   ��Ʈ榡 �~  ��  ��  ��  ��  ��  �@��


//------------------------------------------------------- 2009-3-17
//-----2011/09/28����ϥΪ���
#include  <msp430x20x3.h>
#include <string.h>
bool is_leapyear = false;   //�P�_�|�~


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
_DINT();     // �������_

 InitTimeClock();
  P1DIR |=  (BIT0+BIT6+BIT3+BIT4);          // Set pin to output direction
  P1DIR &= ~(BIT1+BIT2+BIT7);               // Set pin to input direction
  P1OUT&=~BIT4;
  P1OUT&=~BIT6;                             //p1.6 �@�}�l ��low

  P2SEL &= ~BIT6;                           //�]�w P2.6���@��IO�γ~
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
_EINT();    //�}�l���_
   while(1){

     if( (P1IN&BIT1)==BIT1 )     //----SEACLOCK �e��ƥX�h
     { 
     
       memcpy(time_str_buff,time_str,7);
       
      // for(int j=0;j<7;j++)time_str_buff[j]=time_str[j];
       //-----------------------�}�l�e
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
       _DINT();     // �������_
       getTime_init();
       get_time();   //  ���GPS�ɶ��r��
        
       //----- �P�B�B�z
       while((P2IN&BIT6)!=BIT6);    // ���� SeaScan  �� 1pps  ��H��L
       while((P2IN&BIT6)==BIT6);    //�]�� SeaScan �� 1pps�w�g�PGPS�P�B�L
       time_str[0] = -1;
       //--��GPS��L��H   �}�l�� ����SEASCAN��PluS���
       //--�s�������ݭn��PIN19�P�B
       
       _EINT();    //�}�l���_
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
        while(((P1IN&BIT2)!=BIT2)&&(time < TimeOut) ){time++;}   //�N���ƥi�H�e
        time = 0;
       (SendBuf&0x01==0x01) ? P1OUT|=BIT3 : P1OUT&= ~BIT3;
        SendBuf >>=1 ;
        P1OUT&=~BIT4;
        while(((P1IN&BIT2)==BIT2)&&((time < TimeOut))){time++;}   //�N���Ʀ����F
        time = 0;
     }
}
void InitTimeClock(){
  P1DIR &= ~BIT5;
  P1IE  = 0 ;                           //
  P1IES =0;                             //
  P1IFG =0;                             //
  P1IE  |= BIT5;                        // Set P1.5 interrupt
  P1IES &= ~BIT5;                        // ��ܤ��_�Ҧ�(0 ��Lo��HiĲ�o )
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
         if( ((time_str[6]%4)==0) &&((time_str[6]%100)!=0) || (time_str[6]%400)==0){  //--//-�|�~   2�릳29��
                    is_leapyear = true;      }
         else{      is_leapyear = false;  } 
   }
     P1IFG &= ~BIT5;                         // ���_�X���Ȧs��(0 �L���_ 1 ���_���ݳB�z)
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
   
   
  if( ((time_str[6]%4)==0) &&((time_str[6]%100)!=0) || (time_str[6]%400)==0){  //--//-�|�~   2�릳29��
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