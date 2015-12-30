// 2015 1230 �ץ�--- PPS OUTPUT --  ���}��PIN�}����X�Ҧ�  �@�}�l�۰ʳ]�w�����SEASCAN PPS
//******************************************************************************
// 2012 1225 P1OUT ���}����   �ϥήɽ�����~  �ˬd�O�_�|�v�T�T��
// 2012 09��覸 �ץ��C�ѷ|���@�⦸��ɶ����~��Bug
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

bool start_send = false;    // 
bool is_leapyear = false;   //�P�_�|�~


void InitTimeClock();
void IO_Send_Byte(int SendBuf);

void getTime_init();
void get_time();
int get_byte();

int delay ;
#define TimeOut 1500
int ms,sec,minute,hour,day,month,year;
int sMs ,sSec, sMinute , sHour, sDay, sMonth, sYear,sMs_2;
int fix_ms = 0;
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
  
  
 ms = 0;  sec = 0 ;   minute = 16 ;  hour = 11;  day = 22;  month = 3;  year = 12;
 delay = 0;
 
  CCTL0 = CCIE;                             // CCR0 interrupt enabled
  CCR0 = 113;
  TACTL = TASSEL_2 + MC_1;                  // SMCLK, upmode
 
_EINT();    //�}�l���_


 //----- �P�B�B�z
       while((P2IN&BIT6)!=BIT6);    // ���� SeaScan  �� 1pps  ��H��L
       while((P2IN&BIT6)==BIT6);    //�]�� SeaScan �� 1pps�w�g�PGPS�P�B�L
       ms = 0;

   while(1){

     if( (P1IN&BIT1)==BIT1 )     //----SEACLOCK �e��ƥX�h
     { 
       start_send = true;
        //-----------------------�}�l�e
        IO_Send_Byte(sYear);
        IO_Send_Byte(sMonth);
        IO_Send_Byte(sDay);
        IO_Send_Byte(sHour);
        IO_Send_Byte(sMinute);
        IO_Send_Byte(sSec);
        IO_Send_Byte(sMs);
        IO_Send_Byte(sMs_2);
        
       start_send = false;
     }
     if((P1IN&BIT7)==BIT7 )
     {
       _DINT();     // �������_
       P1IE  &= ~BIT5;                        // disabled P1.5 interrupt
       
       getTime_init();
       get_time();   //  ���GPS�ɶ��r��
        
       //----- �P�B�B�z
       while((P2IN&BIT6)!=BIT6);    // ���� SeaScan  �� 1pps  ��H��L
       while((P2IN&BIT6)==BIT6);    //�]�� SeaScan �� 1pps�w�g�PGPS�P�B�L
       ms = 0;
       //--��GPS��L��H   �}�l�� ����SEASCAN��PluS���
       //--�s�������ݭn��PIN19�P�B
       
        P1IFG =0;                             //
        P1IE  |= BIT5;                        // enabled P1.5 interrupt
       _EINT();    //�}�l���_
     }


   }

}

// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{
  //P1OUT ^= 0x01;                            // Toggle P1.0
  //CCR0 += 5000;                            // Add Offset to CCR0
  
  fix_ms++;
  
  if(!start_send){
   sMs_2 = fix_ms;
  }
  if(fix_ms>79)fix_ms=79;
}

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
     ms++;
     fix_ms = 0;
     P1OUT &= ~BIT0; //-----for newboard v1.0

  if(ms>=125){
     ms=0;sec++;
     P1OUT |= BIT0;
  }//-----for newboard v1.0
   if(sec>=60){sec=0;minute++;}
   if(minute>=60){minute=0;hour++;}
   if(hour>=24){hour=0;day++;}
   
  if(is_leapyear){
   if(day>29){  if(month==2 ){day=1;month++;}   }   
   if(day>30){  if((month==4)||(month==6)||(month==9)||(month==11)){day=1;month++;}   }
   if(day>31){  day=1;month++;   }
  }
  else{
   if(day>28){  if(month==2 ){day=1;month++;}   }   
   if(day>30){  if((month==4)||(month==6)||(month==9)||(month==11)){day=1;month++;}   }
   if(day>31){  day=1;month++;   }    
  }
   if(month>12){month=1;year++;
         if( ((year%4)==0) &&((year%100)!=0) || (year%400)==0){  //--//-�|�~   2�릳29��
                    is_leapyear = true;      }
         else{      is_leapyear = false;  } 
   }
  
  if(!start_send){
        sMs = ms;
        sSec= sec;
        sMinute = minute;
        sHour = hour;
        sDay = day;
        sMonth = month;
        sYear = year;
  }
  
     P1IFG &= ~BIT5;                         // ���_�X���Ȧs��(0 �L���_ 1 ���_���ݳB�z)
}
//------------------------------------------------------------------------------
void getTime_init(){
  P1OUT &= ~(BIT4+BIT3);    //
}
void get_time(){
   P1OUT |= BIT3;
   year  =  get_byte();
   P1OUT &= ~BIT3;
   month =  get_byte();
   day   =  get_byte();
   hour  =  get_byte();
   minute=  get_byte();
   sec   =  get_byte();
//   ms    =  get_byte();
delay = get_byte();
   ms    =  0;
   
   
  if( ((year%4)==0) &&((year%100)!=0) || (year%400)==0){  //--//-�|�~   2�릳29��
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