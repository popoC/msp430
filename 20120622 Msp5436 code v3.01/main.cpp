//-------------P6---------------------------------------------------------------
//       P1.5
//         |
//      ___↓_                    ________________             _____
//     |      |P1.3  ->  P6.1    |                |<---  Rx   |     |
//     |      |P1.4  ->  P6.2    |                |--->  Tx   |  PC |
//     |      |P1.1  <-  P6.3    |                |--->  Gnd  |_____|
//     | 2013 |P1.2  <-  P6.4    |    430f5436     |
//     |      |                  |                |
//     |      |                  |                |
//     |______|P1.7  <-  P6.7    |________________|
//------------------------------------------------------------------------------
// 2012 1224 add ms_2 
// 2012 0807 fix TIMEOUT 
// 2012 0803 fix 
//
//
//
//----尚未處理 紀錄時時間輸出 確認ADS1222 轉換時間與gettime()
#include  <msp430x54x.h>
#include "uart.h"
#include <stdio.h>
#include "main.h"
#include "spi.h"
#include "sdhc.h" 

#define Update_Per_Block 30
int Update_Cont = 0;
bool StartRecord = false;
long int wait_for_start = 0;

int OUTPUT_MODE = LOGGER_TIME;

void Crystal_Init();   
//----------for seascan---------------------------------------------------------
#define TimeOut 5000
   void getTime_init();
   void get_time();    // ----- 2.56ms about~
   void IO_Send_Byte(int SendBuf);
    int get_byte();
                  
  unsigned char RTC_Serial[8]; // int ms,sec,minute,hour,day,month,year;  2012 1224 fix
  bool Set_RTC_Flag = false;                  
                    
//----------for rs232-----------------------------------------------------------
char COM2_Command[COM_BUF_Size];
char string[150];
void delay_ms(int del);

//----------for ADS1222---------------------------------------------------------
unsigned long DATA[Channel_N]={0} ;                        //資料暫存區
bool ADS1222_Interrupt_Flag = false;
   void ADS1222Init();
   void ADS1222_STANDBY_MODE();
    int ADS1222_SELF_CALIBRATION();
                     int First_Data = 0;
                     int DRDY_ready = 0;
                     int BITN[8] = {0x0001,0x0002,0x0004,0x0008,0x0010,0x0020,0x0040,0x0080};
//----------for sd card---------------------------------------------------------
       //--- format table
 unsigned char Save_flag = 1;
 int total;
u32 startblock, endblock;
u32 blockaddress, modemaddress = 0;
 
 
int check_SD_card = 1;
int SD_INIT();
int do_sd_initialize (sd_context_t *sdc);
int read_SD_format();
void do_storage_2();
void update_table(void);

//----------for compress v1.0    2010/07/09-------------------------------------                     
   int data_in_block = 0;                  // ---   修改存放位子 2012 1224
   int save_address = 0;                  
   unsigned long Data_Buff[Channel_N];                
   int compress(unsigned long *ADS1222_DATA,unsigned char *SAVE_BUFF,int AddressN);  //---回傳壓縮 暫存區所用空間 
   int CheckDiff(unsigned long *data,unsigned long d1,unsigned long d2,char ch);                     
   int combine(unsigned char *SAVE_BUFF,unsigned long data,int addresscount,int datacount);          
                     
//------------------------------------------------------------------------------
  
    int out_timer_counter = 0;             
//------------------------------------------------------------------------------
void main(void)
{

   WDTCTL = WDTPW + WDTHOLD;                    // Stop WDT

   
   
   
   
   P1DIR = BIT0 +BIT1 ;
   P1OUT |= BIT0;                           //   爺亨版

   Crystal_Init();                          // 震盪器初始化
   UART_Init(COM2);                         // Rs232初始化    
   getTime_init();                          // MSP430F2013 connect
   ADS1222Init();                           // ADS1222初始化
 
  while(!StartRecord){
           __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, enable interrupts 此處中斷大約100Hz
           
           wait_for_start++;
           if(wait_for_start > 180000)StartRecord = true;   //  自動啟動裝置 大約三十分鐘
           
             for(int d1elAy=0;d1elAy<1600;d1elAy++);    //--delay 
                if(First_Data < 5){ First_Data += ADS1222_SELF_CALIBRATION();}
                else{ ADS1222_STANDBY_MODE(); }
           
                
                
                if(Set_RTC_Flag){
                 Set_RTC_Flag = false;
                   P6OUT|=BIT7 ;
                   //set time to msp430f2013
                   IO_Send_Byte(RTC_Serial[0]);
                   P6OUT&= ~BIT7;
                   for(int i=1;i<7;i++) IO_Send_Byte(RTC_Serial[i]); 
                }   
                
         switch(OUTPUT_MODE){
           case LOGGER_TIME:
             out_timer_counter++;
             if(out_timer_counter>=10){
               out_timer_counter = 0;
                     string[0] = 't';                  //資料型態 fix 2012 1224
                     string[9] = 0;  string[10] = 1;  
              get_time();      memcpy(&(string[1]),RTC_Serial,8);
              RS232_Send_Char(string,11,COM2);
             }
             break;      
           case AD_DATA:
                for(int m=0;m<Channel_N;m++)
                {
                   string[0] = 'd';                   //資料型態
                   string[1] = (DATA[m] >> 16 ) & 0xff ;
                   string[2] = (DATA[m] >> 8 ) & 0xff;
                   string[3] = (DATA[m]) & 0xff;
                   string[4] =  m;  string[5] =  '\r';
                   RS232_Send_Char(string,7,COM2);  
                }
             break;
          case RE_SET:
            OUTPUT_MODE = 0;  WDTCTL = WDT_ARST_1000;  //--開狗    
           break;    
         }    
      
  }
  //-------------------------------- start record-----------------------------
  
  get_time();
  P2OUT &= ~(BIT0) ;  // ~確保裝置正常工作  因為啟動紀錄後 ADS1222的SCLK會變成被動往下
  
  P1OUT &= ~0x01;
  
  if(!SD_INIT()){return;}                 // sd card 初始化
  if(read_SD_format()){return;}           // read SD card format 
  
  
  
  while(StartRecord){
  
               __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, enable interrupts
               
    if(ADS1222_Interrupt_Flag){
    
           ADS1222_Interrupt_Flag = false;

           Update_Cont++;


           if((Update_Cont%6) == 0){
            
             P1OUT |= 0x01;
              get_time();  
              memcpy(&(string[1]),RTC_Serial,8);
             RS232_Send_Char(string,11,COM2);
              P1OUT &= ~0x01;
           
           }           

           if(Update_Cont == Update_Per_Block){
             Update_Cont = 0;
             
             update_table();

            
           }

           if(check_SD_card)WDTCTL = WDT_ARST_1000;  //--開狗
         
           do_storage_2();           //

    }

    if(OUTPUT_MODE==RE_SET){
        OUTPUT_MODE = 0; 
       WDTCTL = WDT_ARST_1000;  //--開狗 RESET
    }      
    
    
  }
  
   __no_operation();                          // For debugger                   
}
//----------------------------------------------------------------------------
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
 
  P4SEL = 0x02;                               // P4 option select 
  P4DIR = 0x02;                               // P4 outputs  1.6666MHz.......
  TBCCR0 = 5;                                 // PWM Period
  TBCCTL1 = OUTMOD_4;                         // CCR1 reset/set
  TBCCR1 = 2;                                 // CCR1 PWM Duty Cycle	

  TBCTL = TBSSEL_2 + MC_1 + TBCLR;            // SMCLK, upmode, clear TBR    
  
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
  
  
  if(StartRecord ){  
    
      for(int d1elAy=0;d1elAy<1600;d1elAy++);    //--delay 
      ADS1222_STANDBY_MODE();

 

           if(Save_flag == 1){      save_address =  compress(DATA,sd_buffer1,save_address);    }
           if(Save_flag == 2){      save_address =  compress(DATA,sd_buffer2,save_address);    }
             //  P1OUT &= ~BIT1;   0.35~0.4ms compress run time
  
           
           if(save_address >= 990 ) {          //-- 1024 - (Channel_N*4*2+2) 確保空間足夠  以4bit為一單位!!! 所以必須*2
              save_address = 0;                //---block滿囉~~~
    
               P2OUT &= ~(BIT0) ; // 開始AD轉換 standby mode  2012 0803
              get_time();         // 擷取時間囉~~2012/07/06
              Save_flag++;  
             
              if(Save_flag == 2){        sd_buffer1[8] =  data_in_block ;        data_in_block = 0;             }
              else{        sd_buffer2[8] =  data_in_block ;        data_in_block = 0;          Save_flag=1;     }
              

               ADS1222_Interrupt_Flag = true;
               P2IFG &= ~(BIT3+BIT2+BIT1+BIT4);                   // 中斷旗號暫存器(0 無中斷 1 中斷等待處理)         
             __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
           }      
           else{
               P2OUT &= ~(BIT0) ; // 開始AD轉換 standby mode 2012 0803
               P2IFG &= ~(BIT3+BIT2+BIT1+BIT4);                   // 中斷旗號暫存器(0 無中斷 1 中斷等待處理)         
           }
  }
  else{
                P2IFG &= ~(BIT3+BIT2+BIT1+BIT4);                   // 中斷旗號暫存器(0 無中斷 1 中斷等待處理)         
                __bic_SR_register_on_exit(LPM0_bits); // Exit LPM0
    
  }  

  
}
//------------------------------------------------------------------------------
int ADS1222_SELF_CALIBRATION()
{
int scLk,ch_n,delAy;
long delay_for_calibration;
  for(scLk=1;scLk<=24;scLk++){
    P2OUT |= (BIT0) ;                  //--同步要求輸出資料(SCLK)-----------上源觸發
    for(delAy=0;delAy<20;delAy++);                //DELAY

       for(ch_n=0; ch_n<Channel_N; ch_n++){
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
    
       for(ch_n=0; ch_n<Channel_N; ch_n++){
           if((P2IN&BITN[ch_n+1])==BITN[ch_n+1]){ DATA[ch_n] =( DATA[ch_n]<< 1) | 1;}
           else{ DATA[ch_n] =( DATA[ch_n]<< 1) | 0;}
       }

   if(scLk!=24){
    P2OUT &= ~(BIT0) ;                  //--輸出資料結束--------------通知AD資料收集結束
    for(delAy=0;delAy<25;delAy++);
   }
  }

   for(ch_n=0; ch_n<Channel_N; ch_n++){
     if( (DATA[ch_n]&0x00FFFFFF) <= 0x007FFFFF){      //----2010 / 08/17 修改AD輸出格式 讓輸出0點附近連續
         DATA[ch_n] += 0x00800000; 
     }
     else{
         DATA[ch_n] -= 0x00800000;
     }
   }
 
  if(!StartRecord){
   for(delAy=0;delAy<1500;delAy++);    //--STANDBY MODE
   P2OUT &= ~(BIT0) ;
  }
       
}
//------------------------------------------------------------------------------
void do_storage_2()
{
            if(Save_flag == 2){
             check_SD_card = sd_write_block(&sdc, blockaddress, sd_buffer1);
                   sd_wait_notbusy (&sdc);
              for(int i=0;i<512;i++)sd_buffer1[i]=0;
            }
            else{
            check_SD_card =  sd_write_block (&sdc, blockaddress, sd_buffer2);
                  sd_wait_notbusy (&sdc);  
              for(int i=0;i<512;i++)sd_buffer2[i]=0;
            }
                    // 8G SD card limit -> 15660032 ~~ 15660000
                    // 32G               ->63078400 ~~
                                      
                  if( blockaddress >= 63078300)
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
        endblock = blockaddress-1;
        sd_format_table[16*(total)+4] = (unsigned char)(endblock >> 24);
        sd_format_table[16*(total)+5] = (unsigned char)(endblock >> 16);
        sd_format_table[16*(total)+6] = (unsigned char)(endblock >> 8);
        sd_format_table[16*(total)+7] = (unsigned char)(endblock);

        sd_write_block (&sdc, 0, sd_format_table);
        sd_wait_notbusy (&sdc);
}

int do_sd_initialize (sd_context_t *sdc)
{
	/* Initialize the SPI controller */
	spi_initialize();
	/* Set the maximum SPI clock rate possible */
	spi_set_divisor(PERIPH_CLOCKRATE/400000);
	/* Initialization OK? */
	if (sd_initialize(sdc) != 1)
		return 0;
        spi_set_divisor(2);   //---- 2011 0905 spi full speed
	return 1;
}

int SD_INIT()
{
        int j, ok = 0;
        
  	/* Set some reasonable values for the timeouts. */
        sdc.timeout_write = PERIPH_CLOCKRATE/8;
        sdc.timeout_read = PERIPH_CLOCKRATE/20;
        sdc.busyflag = 0;    

        for (j=0; j<SD_INIT_TRY && ok != 1; j++)
	{
	    ok = do_sd_initialize(&sdc);
	}
        
        return ok;
}

int read_SD_format(){
        // 讀取 format table
        sd_read_block(&sdc, 0, sd_format_table);
        sd_wait_notbusy (&sdc);
        // 讀取資料筆數
        total = sd_format_table[0];
        total = (total << 8) + sd_format_table[1];
        // 讀取最後一筆的 endblock
        startblock = sd_format_table[16*(total)+4];
        startblock = (startblock << 8 ) + sd_format_table[16*(total)+5];
        startblock = (startblock << 8 ) + sd_format_table[16*(total)+6];
        startblock = (startblock << 8 ) + sd_format_table[16*(total)+7];
        if(startblock<=100)startblock=100;
        // 寫入資料筆數
        total ++;
        sd_format_table[0] = total >> 8;
        sd_format_table[1] = total & 0xFF;  //  0x0F-> 0xFF------ 2010 10/06..... 錯誤修正
        // 設定目前的起始與結尾block
        startblock ++;
        blockaddress = startblock;
        endblock = startblock;
        sd_format_table[16*(total)+0] = (unsigned char)(startblock >> 24);
        sd_format_table[16*(total)+1] = (unsigned char)(startblock >> 16);
        sd_format_table[16*(total)+2] = (unsigned char)(startblock >> 8);
        sd_format_table[16*(total)+3] = (unsigned char)(startblock);
        sd_format_table[16*(total)+4] = (unsigned char)(endblock >> 24);
        sd_format_table[16*(total)+5] = (unsigned char)(endblock >> 16);
        sd_format_table[16*(total)+6] = (unsigned char)(endblock >> 8);
        sd_format_table[16*(total)+7] = (unsigned char)(endblock);

//       check_SD_card = 
        sd_write_block (&sdc, 0, sd_format_table);
        sd_wait_notbusy (&sdc);
        return 0;
}
//---------------------------------------------------------------------------------------
int combine(unsigned char *SAVE_BUFF,unsigned long data,int addresscount,int datacount){
    //-----輸入   addresscount 單位為4bit = 0.5byte
   int star = addresscount/2;
   int v=0;

   if(addresscount%2){
     for(v=0;v<datacount/2;v++){
        ((4*(7-(2*v))) < 0 ) ? SAVE_BUFF[star+v] |= data <<4 :
                               SAVE_BUFF[star+v] |= (data>>4*(7-(2*v)));
     }
     if(datacount%2){
        ((4*(7-(2*v))) < 0 ) ? SAVE_BUFF[star+v] |= data <<4 :
                               SAVE_BUFF[star+v] |= (data>>4*(7-(2*v)));
     }
   }
   else{
     for(v=0;v<datacount/2;v++){
      ((4*(6-(2*v))) < 0 ) ? SAVE_BUFF[star+v] |= data <<4 :
                             SAVE_BUFF[star+v] |= (data>>4*(6-(2*v)));
     }
     if(datacount%2){
      ((4*(6-(2*v))) < 0 ) ? SAVE_BUFF[star+v] |= data <<4 :
                             SAVE_BUFF[star+v] |= (data>>4*(6-(2*v)));
     }
   }
    return(addresscount+datacount);
}
//------------------------------------------------------------------------------
int CheckDiff(unsigned long *data,unsigned long d1,unsigned long d2,char ch){

            int count = 0;
   unsigned long s3 = d1^d2;
   unsigned long index = 0;
   unsigned long mask = 0xf00000;
   for(int i=0;i<6;i++){
     index<<=1;
     if((s3 & mask) !=0){
        index|=0x01;
       *data |= ((s3 & mask)>>(5-i)*4);
       count++;
       *data <<= 4;
     }
     mask >>= 4;
   }
   index = index | ch;    //--ch1

   *data >>= 4;
   *data = (index<<24) | *data << (6-count)*4;
        return(count);
}

//----------------------------------------------------------------------------

//------------------------------------------------------------------------------
int compress(unsigned long *ADS1222_DATA,unsigned char *SAVE_BUFF,int AddressN){
   //------------   1~7  7個bytes      資料"開始"時間(舊版本為BLOCK滿的時間)
   //------------   8    1byte 記錄block內含資料數目   //------------   -2   BLOCK最後一個BYTE為 checksum
  int k;
  unsigned long compressdata=0;
   
  if(AddressN==0){
     //    get_time();   2012 1224 毫秒單位多增加一byte
          memcpy(SAVE_BUFF,RTC_Serial,8);                 // 更新時間
          for(k=0;k<Channel_N;k++){
           SAVE_BUFF[4*k +9]  = k+1;
           SAVE_BUFF[4*k +10]  =(unsigned char) (ADS1222_DATA[k]>>16);
           SAVE_BUFF[4*k +11]  =(unsigned char) (ADS1222_DATA[k]>>8);
           SAVE_BUFF[4*k +12] =(unsigned char) (ADS1222_DATA[k]);
           Data_Buff[k] = ADS1222_DATA[k];
          }
        data_in_block++;  
        return(Channel_N*8 + 18);   // 2012 1224
  }
       int cnt =    CheckDiff(&compressdata,ADS1222_DATA[0],Data_Buff[0],0x40);
           AddressN =  combine(SAVE_BUFF,compressdata,AddressN,cnt+2);
           compressdata =0;
           
           cnt =    CheckDiff(&compressdata,ADS1222_DATA[1],Data_Buff[1],0x80);
           AddressN =  combine(SAVE_BUFF,compressdata,AddressN,cnt+2);
           compressdata=0;
           
           cnt =    CheckDiff(&compressdata,ADS1222_DATA[2],Data_Buff[2],0xc0);
           AddressN =  combine(SAVE_BUFF,compressdata,AddressN,cnt+2);
           compressdata=0;
  
           cnt =    CheckDiff(&compressdata,ADS1222_DATA[3],Data_Buff[3],0x00);
           AddressN =  combine(SAVE_BUFF,compressdata,AddressN,cnt+2);
           compressdata=0;
  
   for(k=0;k<Channel_N;k++)Data_Buff[k] = ADS1222_DATA[k];

   data_in_block++;
  return(AddressN);
  
  
}



//----------------------------------------------------------------------------
void delay_ms(int del){
int dt;
  for(int delAy=0;delAy<del;delAy++)    //--delay 
    for(dt=0;dt<4000;dt++);
}
//----------------------------------------------------------------------------

void getTime_init(){
  P6DIR |= (BIT3+BIT4+BIT7);      //out   BIT7 很重要
  P6DIR &= ~(BIT1+BIT2+BIT6);     //input
  P6OUT &= ~(BIT4+BIT3+BIT7);
}
void get_time(){
   P6OUT |= BIT3;
   RTC_Serial[0] = get_byte();
   P6OUT &= ~BIT3;
 for(int i=1;i<8;i++) RTC_Serial[i] = get_byte();
   
}

int get_byte(){
int buf=0;  int time = 0;
 for(int i=0;i<8;i++){
  while(((P6IN&BIT2)!=BIT2)&&(time < TimeOut)){time++;}
   time = 0;
   P6OUT |= BIT4;
  while(((P6IN&BIT2)==BIT2)&&(time < TimeOut)){time++;}
   time = 0;
   if(P6IN & BIT1) buf+=0x01 <<i;
   P6OUT &= ~BIT4;
 }
 return(buf);
}
//------------------------------------------------------------------------------
void IO_Send_Byte(int SendBuf){
   int SendN = 0;
   int time = 0;
     for(SendN=0;SendN<8;SendN++){
        P6OUT|=BIT4;
        while(((P6IN&BIT2)!=BIT2)&&(time < TimeOut) ){time++;}   //代表資料可以送
        time = 0;
       (SendBuf&0x01==0x01) ? P6OUT|=BIT3 : P6OUT&= ~BIT3;
        SendBuf >>=1 ;
        P6OUT&=~BIT4;
        while(((P6IN&BIT2)==BIT2)&&((time < TimeOut))){time++;}   //代表資料收走了
        time = 0;
     }
}
//------------------------------------------------------------------------------