//--2010/10/16 �[�J���}���ɭ��]�Ѽ� ����RS232��X�\��
//----2010/10/12 �[�JSD---------------------
//----2010/10/08 �[�J�ݪ���---------------------
//----2010/10/07 �[�JSD�d�H�ιq�lù�L
//******************************************************************************
// ***** I/O **** .......4/19
// P1   *************************              
// P2.0     input   interrupt for set or start
// P2.1     input   interrupt for test mode
// P2.2~7   X       for nothing  
// P3.4~7   I/O     for RS232
//    P4.0 P4.1  �}��RELAY     //----�`�N PWM �i�ϥΪ��}�쬰 P4.1~7 
//    P4.2 3 4 5 �i��PWM�ϥ�  ((�ثe�O�l�W6 7 �n���� 2 3�})) ���g 4 5��PIN 
//    ch1  P4.4 P4.5    output  for motorA
//    ch2  P4.2 P4.3    output  for motorB
// P5.
//   P6.0  **�L�k�ϥ� ...�]���}�S���X��((�Ŧ�O�l
//---- AD �Шϥ� P6.1 �}�l             ((�Ŧ�O�l
//******************************************************************************
#include  <msp430x16x.h>
#include <stdio.h>
#include "main.h"
#include "sdhc.h"
#include "spi.h"

#define abs(x)( (x) > 0 ? (x) : (-x))
#define CW  1
#define CCW 0
//------------------------------------------//------------------------------------------
//-----------for SD card format table-------------------------------------------
   int SDCARD_Init();      
   int read_SD_format(); 
   void do_storage(int);  
   void update_table();
   
   static unsigned long int blockaddress = 0;
                        int total;
                        u32 startblock, endblock;
    int check_SD_card = 1;                           
    int Save_Data = 0;
//    unsigned long int Save_timeCount = 21600;  //�C6�p�ɰO���@���ɶ�
//    unsigned long int Save_timeCount = 10;  //�C6�p�ɰO���@���ɶ�
//    unsigned long int countSD_saveTime = 0;
    unsigned  long int Now_X_Axis_1 = 0;
    unsigned  long int Now_Y_Axis_1 = 0;    
    unsigned  long int Now_X_Axis_2 = 0;
    unsigned  long int Now_Y_Axis_2 = 0;    
    unsigned  long int Now_X_Axis = 0;
    unsigned  long int Now_Y_Axis = 0;    
//---------------

    bool WDT_flag;
    bool Leveling=false;
//---------------------------------------------


void MotoControl_Stop(int ch);
void MotoControl_Run(int ch,int cw_ccw);
void PwmSet();
void GoToRange_V2();
void GoHome();

void write_flash_segA();
void read_flash_segA();
//---------------UR----function-------------------------------------------------
   void Rs232_Init();             //---���K�}�Ұ��W�_����
   void UART1_SendByte(unsigned char data);
   void UART1_SendStr(char *str);
//--------------------------------------------------------------------
double Angle_X ;
double Angle_Y ;

int Hand_X_Time = 0,Hand_X_Dir=1,Hand_Y_Time=500,Hand_Y_Dir=0;

//static unsigned  long int outPut_Buf = 1620;
static unsigned int results[2];             // Needs to be global in this example
void AD_Init();
void DoTheAd(int avgtime,bool print);



int pow(int i,int j);
//------�B�zRS232��J�T������ܥ� 
char command[255];  int coMMand_N = 0; int coMMand_Mode =0;  char string[255];
int GetInputVal(int G);
//--------------------------------
int Set_OriginX_flag = 0;
int Set_OriginY_flag = 0;

unsigned long int countTime = 0;
 int ControlMode = 1;             
 
 int AD_AVGtime = 1000;           
 int motor_T_AD = 2500;             
 int Set_OriginX = 500;
 int Set_OriginY = 500;
 int move1deg_T = 380; 
           int frequency_pwm = 200;                
 unsigned  int percent_pwm = 80; 
 int RunTime = 150;

     //frequency_pwm
    // percent_pwm 
              int ReStart_Day = 7;                 // 7.
unsigned long int ReStart_Sec = 604800;  // 8.
unsigned int FirstStart_Sec = 28800;              //9.
int startCount = 1;

//-----------------------------------

bool FirstStart = true;
bool outPutcount = true;    
unsigned long int count_back = 0;


void OpenMotorPower();
void CloseMotorPower();

bool Setting_mode = true;


void All_restart(){

  
  
FirstStart = true;
outPutcount = true;
count_back = 0;
Setting_mode = true;

 ReStart_Day = 7;
 ReStart_Sec = 604800;  
 FirstStart_Sec = 28800;
 startCount = 1;
 
}

//------------------------------------------------------------------------------
void main(void)
{
 
  WDTCTL = WDTPW+WDTHOLD;                   // Stop watchdog timer
  WDT_flag = true;
  
  sd_buffer1[1] = 'Y';
  sd_buffer1[2] = '5';
    
    
  FCTL2 = FWKEY + FSSEL0 + FN0;             // MCLK/2 for Flash Timing Generator

  read_flash_segA();

  
  Rs232_Init();                             //  RS232 ��l��      115200   P3  
  
  P4DIR = 0xff;                             // Set P4. to output direction 
  P4OUT = 0;

  
  TBCTL = TBSSEL_2 + MC_1;                  // SCLK, up-down mode  PWM timeClock
  
  AD_Init();
 
  WDTCTL = WDT_ARST_1000;                   //--�}��
  
  CloseMotorPower();                        //----�}�q���ըt��  
  
     // TimerA �Ұ�Ū��
      TACCTL0 = CCIE;                             // CCR0 interrupt enabled
      TACCR0 = 16384-1;                           // for 0.5 Hz
      TACTL = TASSEL_1 + MC_1;                    // ACLK, contmode  

    //     _BIS_SR(GIE);    

     Setting_mode = true;
     long int cou_delay;     
     
     
    SDCARD_Init();   //--��l��SD�d
   if(read_SD_format()){return;}           // read SD card format 
     
     
     while(Setting_mode){                      //---- setting mode
     
               if(Set_OriginX_flag){
                Set_OriginX_flag = 0;
                OpenMotorPower();
                cou_delay = (long int)motor_T_AD * 1150; //       delay
                while(cou_delay > 0){ cou_delay--;}       
                DoTheAd(AD_AVGtime,1);  
                CloseMotorPower();
                Set_OriginX =  (int)(Angle_X*100);
                write_flash_segA();
              }
              if(Set_OriginY_flag){
                Set_OriginY_flag = 0;
                OpenMotorPower();
                cou_delay = (long int)motor_T_AD * 1150; //       delay
                while(cou_delay > 0){ cou_delay--;}       
                DoTheAd(AD_AVGtime,1);  
                CloseMotorPower();
                Set_OriginY =  (int)(Angle_Y*100);
                write_flash_segA();
              }

           _BIS_SR(LPM0_bits + GIE);    
     }
      
  IE2 &= ~URXIE1 ;                           // Disable USART1 RX interrupt
         ReStart_Sec =  ((unsigned long int)ReStart_Day * 3600);  //--
      //     ReStart_Sec =  60;  //--  �C�j������---y5���ժ���~~ �C1�������@��
  PwmSet();
  


  sd_buffer1[15] = Set_OriginX & 0xff;
  sd_buffer1[16] = ((Set_OriginX>>8) & 0xff);
  sd_buffer1[17] = Set_OriginY & 0xff;
  sd_buffer1[18] = ((Set_OriginY>>8) & 0xff);


   Leveling = true;
while(1)          //-------------- working mode
  {
                           
    if(Leveling){  

      WDTCTL = WDTPW+WDTHOLD;                       // Stop watchdog timer
       TACCTL0 &= ~CCIE;                             // close timer
       
       Leveling = false;

        
      OpenMotorPower();       //----�}�q  
      WDTCTL = WDTPW+WDTHOLD;                       // Stop watchdog timer
      
                long int cou = (long int)motor_T_AD * 1150; //    100Hz   delay
                while(cou > 0){       cou--;   }       
                
      GoToRange_V2();
      GoHome();
     
      Now_X_Axis_2 = Now_X_Axis;
      Now_Y_Axis_2 = Now_Y_Axis;

      
      CloseMotorPower();      //----���q  
//       outPutcount = true;
      
            do_storage(2);
       WDTCTL = WDT_ARST_1000;             
            update_table();
       WDTCTL = WDT_ARST_1000;
        
       TACCTL0 |= CCIE;                             // open timer
    }

/*    
    if(Save_Data==1){
      Save_Data = 0;
      do_storage(1);
      update_table();
    }
 */     
    
    
    _BIS_SR(LPM0_bits + GIE);                 // Enter LPM0, Enable interrupts
 
    
    
      
  }
  
    
    
  
}

// Timer A0 interrupt service routine   //    if(DoAd) ADC12CTL0 |= ADC12SC;  // Start conversion

int divide = 0;
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{          //  **** ***** **** 1Hz
  
  if(WDT_flag)WDTCTL = WDT_ARST_1000;                            //--����

  if(startCount){                                                //---�Ȱ�Ū�� �\��
       divide++;        
       if(divide>1){
            divide=0;
            countTime++; 
//            countSD_saveTime++;
       }      //-.-  �@��
  }

  if(outPutcount && (divide==1)){                                //�C��z�LRS232��X
      FirstStart ?count_back = FirstStart_Sec - countTime : count_back = ReStart_Sec - countTime ;
      sprintf(string,"%5ld s/ per%2d hours  SD:%2d , %c%c \r",count_back,ReStart_Day,check_SD_card,sd_buffer1[1],sd_buffer1[2]); 
      UART1_SendStr(string);         
      //  _BIC_SR_IRQ(LPM0_bits);     
  }
  if( (countTime >= FirstStart_Sec ) && FirstStart){
           outPutcount = false;                //---�����Ĥ@����N���ΦA��X��
             FirstStart = false;
             countTime = 0;
             Setting_mode = false;
             Leveling = true;
           _BIC_SR_IRQ(LPM0_bits);     
   }      

//  if( countSD_saveTime>=Save_timeCount){
//         countSD_saveTime = 0;
//            Save_Data = 1;
//       _BIC_SR_IRQ(LPM0_bits);   
//  }
  
  if( (countTime >= ReStart_Sec) &&(!FirstStart)){
         Leveling = true;
         countTime = 0;
//            Save_Data = 2;
         _BIC_SR_IRQ(LPM0_bits);   
   }

  
}

//------------------------------------------------------------------------------
void Rs232_Init(){
                            //------------P3------------------------------------
                            // P3.0(O) P3.1(I) P3.2(I) P3.3(I) P3.4(I) P3.5(I) P3.6(O) P3.7(I)
                            //   ��      ��      ��      ��      X       X       ��      ��
                            // SS(spi)  SIMO    SOMI    SCLK     t       r      TX      RX
                            //   �|-------SDHC-----------�}       �|Rs232-�}     �|Rs232-�}
                            //------------P3------------------------------------
  P3SEL |= 0xC0;                       // P3.6,7 = USART1 TXD/RXD
  P3SEL |= 0x30;                       // P3.4,5 = USART0 TXD/RXD

//  P3DIR |= 0x30;                       // P3.4,P3.5, outputs
//  P3OUT |= BIT4;
//  P3OUT |= BIT5;
  
  
 volatile unsigned int i;  
  BCSCTL1 &= ~XT2OFF;                       // XT2on
  do{
    IFG1 &= ~OFIFG;                           // Clear OSCFault flag
    for (i = 0xFF; i > 0; i--);               // Time for flag to set
  }
  while ((IFG1 & OFIFG));                   // OSCFault flag still set?

  BCSCTL2 |= SELM_2 + SELS;                 // MCLK= SMCLK= XT2 (safe)
  
  ME2 |= UTXE1 + URXE1;                     // Enable USART1 TXD/RXD
  UCTL1 |= CHAR;                            // 8-bit character
  UTCTL1 |= SSEL1;                          // UCLK = SMCLK
  UBR01 = 0x45;                             // 8Mhz/115200 - 69.44
  UBR11 = 0x00;                             //
  UMCTL1 = 0x2C;                            // modulation
  UCTL1 &= ~SWRST;                          // Initialize USART state machine
  IE2 |= URXIE1 ;                           // Enable USART1 RX interrupt
  IFG2 &= ~UTXIFG1;                         // Clear inital flag on POR

  return;
}
void UART1_SendByte(unsigned char data)
{
        TXBUF1 = data;			          // �o�e�ƾ�
}
void UART1_SendStr(char *str)
{
        while (*str != '\0')
        {
            UART1_SendByte(*str++);	          // �o�e�ƾ�
            while (!(IFG2 & UTXIFG1));
        }
}
//------------------------------------------------------------------------------
int pow(int i,int j){
    int ans = 1;
    for(int k=0;k<j;k++)ans  = ans * i;
    return(ans);
}
//------------------------------------------------------------------------------
int GetInputVal(int G){
  if (G > 6)G=6;
      bool MnM = false;
        int coni = 0;     int ans = 0;
             for(int i=G-1;i>=0;i--){
               if(MnM){
                    ans += (command[i]-48) * pow(10,coni);
                    coni++;
                 }
                 if(command[i]==0x0D)MnM = true;
             }
             return(ans);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void read_flash_segA()
{
  char *Flash_ptr;                          // Flash pointer
  Flash_ptr = (char *) 0x1000;              // Initialize Flash pointer
  // SegA total 128    
  AD_AVGtime = 0;  motor_T_AD = 0;   Set_OriginX = 0;  move1deg_T = 0;
  percent_pwm = 0; frequency_pwm = 0; FirstStart_Sec = 0;Set_OriginY = 0;RunTime = 0;
  ReStart_Day = 0;

    AD_AVGtime += *Flash_ptr++;
    AD_AVGtime += (*Flash_ptr++)<<8;

    motor_T_AD += *Flash_ptr++;
    motor_T_AD += (*Flash_ptr++)<<8;

    Set_OriginX += *Flash_ptr++;
    Set_OriginX += (*Flash_ptr++)<<8;
    
    move1deg_T += *Flash_ptr++;
    move1deg_T += (*Flash_ptr++)<<8;
  
    frequency_pwm += *Flash_ptr++;
    frequency_pwm += (*Flash_ptr++)<<8;

    percent_pwm += *Flash_ptr++;
    percent_pwm += (*Flash_ptr++)<<8;
    
    FirstStart_Sec += *Flash_ptr++;
    FirstStart_Sec += (*Flash_ptr++)<<8;
    
    Set_OriginY += *Flash_ptr++;
    Set_OriginY += (*Flash_ptr++)<<8;
    
    RunTime += *Flash_ptr++;
    RunTime += (*Flash_ptr++)<<8;
      
    ReStart_Day += *Flash_ptr++;
    ReStart_Day += (*Flash_ptr++)<<8;
}
void write_flash_segA()
{
  char *Flash_ptr;                          // Flash pointer

  Flash_ptr = (char *) 0x1000;              // Initialize Flash pointer
  FCTL1 = FWKEY + ERASE;                    // Set Erase bit
  FCTL3 = FWKEY;                            // Clear Lock bit
 
  *Flash_ptr = 0;                           // Dummy write to erase Flash segment

  FCTL1 = FWKEY + WRT;                      // Set WRT bit for write operation
 
  *Flash_ptr++  =  (AD_AVGtime & 0x00ff);
  *Flash_ptr++  =  ((AD_AVGtime>>8)& 0x00ff) ;
  
  *Flash_ptr++  =  (motor_T_AD & 0x00ff);
  *Flash_ptr++  =  ((motor_T_AD>>8)& 0x00ff) ;
  
  *Flash_ptr++  =  (Set_OriginX & 0x00ff);
  *Flash_ptr++  =  ((Set_OriginX>>8)& 0x00ff) ;
  
  *Flash_ptr++  =  (move1deg_T & 0x00ff);
  *Flash_ptr++  =  ((move1deg_T>>8)& 0x00ff) ;

  *Flash_ptr++  =  (frequency_pwm & 0x00ff);
  *Flash_ptr++  =  ((frequency_pwm>>8)& 0x00ff) ;
  
  *Flash_ptr++  =  (percent_pwm & 0x00ff);
  *Flash_ptr++  =  ((percent_pwm>>8)& 0x00ff) ;

  *Flash_ptr++  =  (FirstStart_Sec & 0x00ff);
  *Flash_ptr++  =  ((FirstStart_Sec>>8)& 0x00ff) ;
  
  *Flash_ptr++  =  (Set_OriginY & 0x00ff);
  *Flash_ptr++  =  ((Set_OriginY>>8)& 0x00ff) ;
  
  *Flash_ptr++  =  (RunTime & 0x00ff);
  *Flash_ptr++  =  ((RunTime>>8)& 0x00ff) ;
  
  *Flash_ptr++  =  (ReStart_Day & 0x00ff);
  *Flash_ptr++  =  ((ReStart_Day>>8)& 0x00ff) ;

 
  
  FCTL1 = FWKEY;                            // Clear WRT bit
  FCTL3 = FWKEY + LOCK;                     // Set LOCK bit
}

//-----------------------------------------------------------------------------
void DoTheAd(int avgtime,bool print){
  unsigned  long int outPutX = 0;
  unsigned  long int outPutY = 0;
  
   for(int y=0;y<avgtime;y++)
   {
      ADC12CTL0 |= ADC12SC;                     // Start conversion
      while ((ADC12IFG & BIT2)==0);             //---
      //      _BIS_SR(LPM0_bits + GIE); 
       results[0] = ADC12MEM1;                   // Move results, IFG is cleared
       results[1] = ADC12MEM2;                   // Move results, IFG is cleared
      
      outPutX += results[0];
      outPutY += results[1];
   }
     outPutX = outPutX /avgtime;
     outPutY = outPutY /avgtime;
     
     Angle_X = (20 * ( (double)outPutX / 4096)) - 10 - ((double)Set_OriginX*0.01);
     Angle_Y = (20 * ( (double)outPutY / 4096)) - 10 - ((double)Set_OriginY*0.01);
    

     Now_X_Axis = outPutX;
     Now_Y_Axis = outPutY;
     
     
     
//     if(print){ sprintf(string," the X angle = %2.6f    Y = %2.6f  ",Angle_X,Angle_Y); UART1_SendStr(string);
//                sprintf(string," the X AD    = %05d     Y = %05d  \r\n",(int)outPutX,(int)outPutY); UART1_SendStr(string);
//     }
}


void AD_Init(){
  P6SEL = 0x06;                             // Enable A/D channel inputs
  ADC12CTL0 = ADC12ON+MSC+SHT0_2;           // Turn on ADC12, set sampling time
  ADC12CTL1 = SHP+CONSEQ_1+SSEL1;                 // Use sampling timer, single sequence
  ADC12MCTL1 = INCH_1;                           // ref+=AVcc, channel = A1         ((P6.1
  ADC12MCTL2 = INCH_2+EOS;                       // ref+=AVcc, channel = A2 end seq.((P6.2
  ADC12CTL0 |= ENC;                            // Enable conversions
}
//------------------------------------------------------------------------------
void CloseMotorPower(){ 
    WDT_flag=false;
    WDTCTL = WDT_ARST_1000;
  
      P4OUT |= BIT1;
          for(long int i=0;i<8000;i++){}   //---10ms
      P4OUT &= ~(BIT1);
    WDT_flag=true;
    WDTCTL = WDT_ARST_1000;
}

void OpenMotorPower(){  
    WDT_flag=false;
    WDTCTL = WDT_ARST_1000;
   
       P4OUT |= BIT0;
         for(long int i=0;i<8000;i++){}//     for(long int i=0;i<59600;i++){}
       P4OUT &= ~(BIT0);
    WDT_flag=true;
    WDTCTL = WDT_ARST_1000;
}
//----------------------------------------------------�\��ק�~ 2010/04/17
void GoHome(){

   long int cou = (long int)motor_T_AD * 1150; //    100Hz   delay
   while(cou > 0){       cou--;   }      
   DoTheAd(AD_AVGtime,1);  
   
   double Ang_Buf_X, Ang_Buf_Y;   
   double MoveTime_X, MoveTime_Y;      MoveTime_X = move1deg_T;     MoveTime_Y = move1deg_T;

   long int couX = -1, couY=-1;
   bool closeX,closeY;  

   
   for(int hh=0;hh<RunTime;hh++){
     closeX = false;     closeY = false;
     
        Ang_Buf_X = Angle_X;
        Ang_Buf_Y = Angle_Y;
          
     if( ( (abs(Angle_X)) < 0.1) && ( (abs(Angle_Y)) < 0.1)  )break;
   
     if(( (abs(Angle_X)) >= 0.1) ){
        (Angle_X  < 0) ? MotoControl_Run(1,CW) : MotoControl_Run(1,CCW);
         couX = (long int)abs(MoveTime_X *1150); 
         
       //  sprintf(string," couX is %05ld \r\n",couX); UART1_SendStr(string);              
     }
     if(( (abs(Angle_Y)) >= 0.1) ){
        (Angle_Y  < 0) ? MotoControl_Run(2,CW) : MotoControl_Run(2,CCW);
         couY = (long int)abs(MoveTime_Y *1150); 
       //  sprintf(string," couY is %05ld \r\n",couY); UART1_SendStr(string);              
     }
     
        while(1){
          couX--; couY--;    
               if( (couX <0) && (!closeX)){closeX = true;  MotoControl_Stop(1);} 
               if( (couY <0) && (!closeY)){closeY = true;  MotoControl_Stop(2);}     
               if(closeX && closeY)break;
        }
     
        cou = (long int)motor_T_AD * 1150;   while(cou > 0){       cou--;   }      
        DoTheAd(AD_AVGtime,1);              
   
        if(Angle_X==Ang_Buf_X){MoveTime_X = move1deg_T;}
        else{    MoveTime_X =  (Angle_X * MoveTime_X) / (Angle_X - Ang_Buf_X); }
        if(Angle_Y==Ang_Buf_Y){MoveTime_Y = move1deg_T;}
        else{MoveTime_Y =  (Angle_Y * MoveTime_Y) / (Angle_Y - Ang_Buf_Y);}
       
        if(abs(MoveTime_X) > move1deg_T*5)MoveTime_X = move1deg_T;   //-- �o���F
        if(abs(MoveTime_Y) > move1deg_T*5)MoveTime_Y = move1deg_T;

       // sprintf(string," MoveTime_X is %09f  MoveTime_Y is %09f \r\n",MoveTime_X,MoveTime_Y); UART1_SendStr(string);     
       // sprintf(string," RunTime =  %d   \r\n",hh ); UART1_SendStr(string);     
   
   }
   
   //sprintf(string," End X is %05f  End Y is %05f \r\n",Angle_X,Angle_Y); UART1_SendStr(string);     
   
   
}
//-------------------------------------------------------------------------------
void GoToRange_V2(){
    
  bool enter_range_X = false,enter_range_Y = false;  

 DoTheAd(AD_AVGtime,0);  

 Now_X_Axis_1 = Now_X_Axis;
 Now_Y_Axis_1 = Now_Y_Axis;
 
 (Angle_X  < 0) ? MotoControl_Run(1,CW) : MotoControl_Run(1,CCW);
 (Angle_Y  < 0) ? MotoControl_Run(2,CW) : MotoControl_Run(2,CCW);

 
  long int countTTT = 0;
 while(1){
 
   DoTheAd(AD_AVGtime,0);  
   countTTT++;
   if( ( (abs(Angle_X)) < 1) && !enter_range_X   ){
          enter_range_X = true;
          MotoControl_Stop(1);
      //    sprintf(string," X Into Range  = %32f \r\n",Angle_X); UART1_SendStr(string);
   }
   if( ( (abs(Angle_Y)) < 1) && !enter_range_Y   ){
          enter_range_Y = true;
          MotoControl_Stop(2);
     //     sprintf(string," Y Into Range  = %32f \r\n",Angle_Y); UART1_SendStr(string);  
   }

    if(enter_range_X && enter_range_Y )break;
    if(countTTT > 5000)break;
 }

    //  sprintf(string,"  Run Time is  = %32ld \r\n",countTTT); UART1_SendStr(string);  
 
}
//-----------------------------------------------------------------------------------
void PwmSet(){
  TBCCR0 = 8000000/frequency_pwm;                           // PWM Period
    TBCCTL2 = OUTMOD_7;                         //  reset/set
    TBCCR2 =   ((100-percent_pwm)* (8000000/frequency_pwm))/100; //  PWM duty cycle
    TBCCTL3 = OUTMOD_7;                         //  reset/set
    TBCCR3 =   ((100-percent_pwm)* (8000000/frequency_pwm))/100; //  PWM duty cycle
    TBCCTL4 = OUTMOD_7;                         //  reset/set
    TBCCR4 =   ((100-percent_pwm)* (8000000/frequency_pwm))/100; //  PWM duty cycle
    TBCCTL5 = OUTMOD_7;                         //  reset/set
    TBCCR5 =   ((100-percent_pwm)* (8000000/frequency_pwm))/100; //  PWM duty cycle
}

void MotoControl_Run(int ch,int cw_ccw){
  switch(ch){
  case 1:
    if(cw_ccw==CW){P4SEL |= BIT2; P4OUT |=BIT3;}
    else          {P4SEL |= BIT3; P4OUT |=BIT2;}
    break;  
  case 2:
    if(cw_ccw==CW){P4SEL |= BIT4; P4OUT |=BIT5;}
    else          {P4SEL |= BIT5; P4OUT |=BIT4;}
    break;
  }
}
void MotoControl_Stop(int ch){
  switch(ch){
  case 1:
    P4SEL &= ~(BIT2+BIT3);
    P4OUT &= ~(BIT2+BIT3);
    break;  
  case 2:
    P4SEL &= ~(BIT4+BIT5);
    P4OUT &= ~(BIT4+BIT5);
    break;
  }
}

//------------------------------------------------------------------
//-------------------------���_-------------------------------------------------
#pragma vector=UART1RX_VECTOR
__interrupt void usart1_rx (void)
{
       command[coMMand_N] = RXBUF1;
       coMMand_N++;

       if(command[coMMand_N-1]==0x1B){      //  ESC
         coMMand_Mode = 0;          coMMand_N = 0;
         return;
       }
       if (command[coMMand_N-1]==0x0D){     // Enter
           switch(coMMand_Mode){
           case 0:
               if(command[0] == 'k'){ 

                 coMMand_Mode = 99;
            //    string[0] = '\r' ;string[1] = '\n' ;  
            sprintf(string,"please select the function \r\n");                  UART1_SendStr(string);
            sprintf(string," ------V3.18------\r\n");                                UART1_SendStr(string);
            sprintf(string,"1. ����Ҧ�((���=0 �۰ʦ^�k���I=1 %04d \r\n",ControlMode);       UART1_SendStr(string);
            sprintf(string,"2. ����AD�������� = %04d \r\n",AD_AVGtime);            UART1_SendStr(string);
            sprintf(string,"3. ���F��ʫ�Ū��AD������ɶ� = %04d \r\n",motor_T_AD);             UART1_SendStr(string);
            sprintf(string,"4. ���I_X = %04d \r\n",Set_OriginX);           UART1_SendStr(string);
            sprintf(string,"5. ���I_Y= %04d \r\n",Set_OriginY);            UART1_SendStr(string);
            sprintf(string,"6. ����1�שһݮɶ�(�Ш̾ڰ��F�S�ʭק� = %04d  \r\n",move1deg_T);            UART1_SendStr(string);
            sprintf(string,"7. pwm_�W�v = %06d \r\n",frequency_pwm);       UART1_SendStr(string);
            sprintf(string,"8. pwm_�ʤ��� = %05d (0~100) \r\n",percent_pwm);   UART1_SendStr(string);
            sprintf(string,"9. �۰ʧ�M���I���� = %05d (0~5000) \r\n",RunTime);        UART1_SendStr(string);
            sprintf(string,"a. ��ʱ��� ����X�b�ɶ� = %05d (0~65000) \r\n",Hand_X_Time);UART1_SendStr(string);
            sprintf(string,"b. ��ʱ��� ����X�b��V = %05d (0~65000) \r\n",Hand_X_Dir); UART1_SendStr(string);
            sprintf(string,"w. ��ʱ��� ����Y�b�ɶ� = %05d (0~65000) \r\n",Hand_Y_Time);UART1_SendStr(string);
            sprintf(string,"d. ��ʱ��� ����Y�b��V = %05d (0~65000) \r\n",Hand_Y_Dir); UART1_SendStr(string);

            sprintf(string,"e. �Ĥ@���Ұʮɶ� = %05d (0~65000) ��\r\n",FirstStart_Sec);UART1_SendStr(string);
            sprintf(string,"f. �C�j%02d�p�ɭ��s�Ұ�                \r\n",ReStart_Day); UART1_SendStr(string);
            sprintf(string,"g. �˼ƼȰ� (0�Ȱ� 1�~�� ) \r\n");UART1_SendStr(string);

            sprintf(string," --------------------\r\n");         UART1_SendStr(string);
               }  
             
               break;
           case 99:
             
                coMMand_Mode  =  command[0];
              switch(coMMand_Mode){
               case '1':             
                    sprintf(string," ControlMode  = (�w�]��1");       UART1_SendStr(string);
                break;
               case '2':
                    sprintf(string," AD_AVGtime    =   ");             UART1_SendStr(string);
                break;
               case '3':
                    sprintf(string," motor_T_AD =   ");             UART1_SendStr(string);
                break;
               case '4':
                    sprintf(string," Set_OriginX=   ");             UART1_SendStr(string);
                break;
               case '5':
                    sprintf(string," Set_OriginY =   ");        UART1_SendStr(string);
                break;
               case '6':
                    sprintf(string," move1deg_T =   ");        UART1_SendStr(string);
                break;
               case '7':
                    sprintf(string," frequency_pwm =   ");      UART1_SendStr(string);
                break;
               case '8':
                    sprintf(string," percent_pwm =   ");       UART1_SendStr(string);
                break;
               case '9':
                    sprintf(string," RunTime =   ");       UART1_SendStr(string);
                break;
               case 'a':
                    sprintf(string," Hand_X_Time =   ");       UART1_SendStr(string);
                break;
               case 'b':
                    sprintf(string," Hand_X_Dir =   ");       UART1_SendStr(string);
                break;
               case 'w':
                    sprintf(string," Hand_Y_Time =   ");       UART1_SendStr(string);
                break;
               case 'd':
                    sprintf(string," Hand_Y_Dir =   ");       UART1_SendStr(string);
                break;
               case 'e':
                    sprintf(string," FirstStart_Sec =   ");       UART1_SendStr(string);
                break;
               case 'f':
                    sprintf(string," ReStart_Day =   ");       UART1_SendStr(string);
                break;
               case 'g':
                    sprintf(string," 0 oR 1 =   ");       UART1_SendStr(string);
                break;
                
              }
              break;
           case '1':
              ControlMode =GetInputVal(coMMand_N);
              break;
           case '2':
              AD_AVGtime = GetInputVal(coMMand_N);              write_flash_segA();
             break;
           case '3':
              motor_T_AD = GetInputVal(coMMand_N);              write_flash_segA();
             break;
           case '4':
              Set_OriginX = GetInputVal(coMMand_N);
              if(Set_OriginX !=0)Set_OriginX_flag = 1;        _BIC_SR_IRQ(LPM0_bits);                        
             break;
           case '5':
              Set_OriginY = GetInputVal(coMMand_N);
              if(Set_OriginY !=0)Set_OriginY_flag = 1;        _BIC_SR_IRQ(LPM0_bits); 
             break;
           case '6':
              move1deg_T = GetInputVal(coMMand_N);               write_flash_segA();              
             break;
           case '7':
             frequency_pwm = GetInputVal(coMMand_N);             write_flash_segA();  
             break;
           case '8':
             percent_pwm = GetInputVal(coMMand_N);               write_flash_segA();  
             break;
           case '9':
             RunTime = GetInputVal(coMMand_N);                   write_flash_segA();  
             break;             
           case 'a':
             Hand_X_Time = GetInputVal(coMMand_N);
             break;
           case 'b':
             Hand_X_Dir = GetInputVal(coMMand_N);
             break;
           case 'w':
             Hand_Y_Time = GetInputVal(coMMand_N);
             break;
           case 'd':
             Hand_Y_Dir = GetInputVal(coMMand_N);
             break;

           case 'e':
             FirstStart_Sec = GetInputVal(coMMand_N);            write_flash_segA();  
             break;
           case 'f':
             ReStart_Day = GetInputVal(coMMand_N);               write_flash_segA();   
             break;
           case 'g':
             startCount = GetInputVal(coMMand_N);
             break;
             
           }
           coMMand_N = 0;
       }

}
//-------------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int do_sd_initialize (sd_context_t *sdc)
{
	spi_initialize();   // Initialize the SPI controller
	// Start out with a slow SPI clock, 400kHz, as required by the SD spec
	spi_set_divisor(PERIPH_CLOCKRATE/400000);
	// Initialization OK?
	if (sd_initialize(sdc) != 1)return 0;
	spi_set_divisor(2);return 1;  // Set the maximum SPI clock rate possible	
}
//------------------------------------------------------------------------------
void do_storage(int mode)
{
  
  if(mode==1)  {
             sd_buffer1[0] = 'D';
  }else{
             sd_buffer1[0] = 'M';
  }
  
  
//             sd_buffer1[3] = countSD_saveTime & 0x00ff;
//             sd_buffer1[4] = ((countSD_saveTime>>8) & 0x00ff);

             sd_buffer1[5] = Now_X_Axis_1 & 0x00ff;
             sd_buffer1[6] = ((Now_X_Axis_1>>8) & 0x00ff);

             sd_buffer1[7] = Now_Y_Axis_1 & 0x00ff;
             sd_buffer1[8] = ((Now_Y_Axis_1>>8) & 0x00ff);

             sd_buffer1[9] = Now_X_Axis_2 & 0x00ff;
             sd_buffer1[10] = ((Now_X_Axis_2>>8) & 0x00ff);

             sd_buffer1[11] = Now_Y_Axis_2 & 0x00ff;
             sd_buffer1[12] = ((Now_Y_Axis_2>>8) & 0x00ff);
  
             sd_buffer1[13] = countTime & 0x00ff;
             sd_buffer1[14] = ((countTime>>8) & 0x00ff);

             sd_buffer1[15] = Set_OriginX & 0x00ff;
             sd_buffer1[16] = ((Set_OriginX>>8) & 0x00ff);

             sd_buffer1[17] = Set_OriginY & 0x00ff;
             sd_buffer1[18] = ((Set_OriginY>>8) & 0x00ff);
             
             
             
             check_SD_card = sd_write_block(&sdc, blockaddress, sd_buffer1);
                   sd_wait_notbusy (&sdc);
         //     for(int i=0;i<512;i++)sd_buffer1[i]=0;

                   // 8G SD card limit -> 15660032 ~~ 15660000
                   // 16G               ->31388672 ~~ 31388600
                   // 32G               ->63078400 ~~
                                      
                  if( blockaddress >= 15660032)
                  {
                    //-do something for protect data
                    return ;
                  }
                  else{
                    blockaddress++;
                  }

}
//------------------------------------------------------------------------------
void update_table()
{
           endblock = blockaddress;
           sd_format_table[16*(total)+4] = (unsigned char)(endblock >> 24);
           sd_format_table[16*(total)+5] = (unsigned char)(endblock >> 16);
           sd_format_table[16*(total)+6] = (unsigned char)(endblock >> 8);
           sd_format_table[16*(total)+7] = (unsigned char)(endblock);
          check_SD_card = sd_write_block (&sdc, 0, sd_format_table);
           sd_wait_notbusy (&sdc);                  
}
//------------------------------------------------------------------------------
int read_SD_format(){
        // Ū�� format table
        sd_read_block(&sdc, 0, sd_format_table);
        sd_wait_notbusy (&sdc);
        // Ū����Ƶ���
        total = sd_format_table[0];
        total = (total << 8) + sd_format_table[1];
        
        
       
        
        // Ū���̫�@���� endblock
        startblock = sd_format_table[16*(total)+4];
        startblock = (startblock << 8 ) + sd_format_table[16*(total)+5];
        startblock = (startblock << 8 ) + sd_format_table[16*(total)+6];
        startblock = (startblock << 8 ) + sd_format_table[16*(total)+7];
        if(startblock<=100)startblock=100;
        // �g�J��Ƶ���
        total ++;
       
         if(total>31){
          total = 1;
        }
        
        sd_format_table[0] = total >> 8;
        sd_format_table[1] = total & 0xFF;  //  0x0F-> 0xFF------ 2010 10/06..... ���~�ץ�
        // �]�w�ثe���_�l�P����block
        startblock ++;
        blockaddress = startblock;
        endblock = startblock;
       
       
        
                                 //--½��
        
        sd_format_table[16*(total)+0] = (unsigned char)(startblock >> 24);
        sd_format_table[16*(total)+1] = (unsigned char)(startblock >> 16);
        sd_format_table[16*(total)+2] = (unsigned char)(startblock >> 8);
        sd_format_table[16*(total)+3] = (unsigned char)(startblock);
        sd_format_table[16*(total)+4] = (unsigned char)(endblock >> 24);
        sd_format_table[16*(total)+5] = (unsigned char)(endblock >> 16);
        sd_format_table[16*(total)+6] = (unsigned char)(endblock >> 8);
        sd_format_table[16*(total)+7] = (unsigned char)(endblock);

       check_SD_card = sd_write_block (&sdc, 0, sd_format_table);
        sd_wait_notbusy (&sdc);
        return 0;
}
//------------------------------------------------------------------------------
int SDCARD_Init(){
         int j, ok=0;
      // Set some reasonable values for the timeouts.
	sdc.timeout_write = PERIPH_CLOCKRATE/8;
	sdc.timeout_read = PERIPH_CLOCKRATE/20;
	sdc.busyflag = 0;
        for (j=0; j<SD_INIT_TRY && ok != 1; j++){
           ok = do_sd_initialize(&sdc);
        }
        if( ok != 1 ){ return 1;}
        else{
        return 2;            //----- SD CARD is on board;
        }
}
//------------------------------------------------------------------------------