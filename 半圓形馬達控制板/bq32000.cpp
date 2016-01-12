//#include <MSP430x54x.h>
#include <MSP430x16x.h>
#include "bq32000.h"


void I2C_Set_sda_H(){

    SDA_select_out;
    SDA_out_H;
   
    _NOP();
    _NOP();
    return;
}
void I2C_Set_sda_L(){
    SDA_select_out;
    SDA_out_L;
    _NOP();
    _NOP();
    return;
}
void I2C_Set_sck_H(){
    SCL_out_H;
    _NOP();
    _NOP();
    return;
}
void I2C_Set_sck_L(){
    SCL_out_L;
    _NOP();
    _NOP();
    return;
}
void I2C_START(){
  int i;
  I2C_Set_sda_H();
  for(i=5;i>0;i--);
  I2C_Set_sck_H();
  for(i=5;i>0;i--);
  I2C_Set_sda_L();
  for(i=5;i>0;i--);
  I2C_Set_sck_L();
  for(i=5;i>0;i--);
  return;
}
void I2C_STOP(){
  int i;

  I2C_Set_sda_L();
  for(i=5;i>0;i--);
  I2C_Set_sck_H();
  for(i=5;i>0;i--);

  I2C_Set_sda_H();
  for(i=5;i>0;i--);
  I2C_Set_sck_L();
  for(i=5;i>0;i--);

  I2C_Set_sck_H();
  Delay_ms(1);


  return;
}

void Delay_ms(unsigned long nValue){

  unsigned long nCount;
  int i;
  unsigned long j;
  nCount = 2667;
  for(i=nValue;i>0;i--){
  for(j=nCount;j>0;j--);
  }
 return;
}

void I2C_TxHtoL(int nValue){
  int i,j;
  for( i=0; i<8;i++){
    if(nValue &0x80)
      I2C_Set_sda_H();
    else
      I2C_Set_sda_L();
    
    for(j =30;j>0;j--);
    I2C_Set_sck_H();
    nValue <<=1;
    for(j =30;j>0;j--);
    I2C_Set_sck_L();
  }
    return;  
}

int I2C_RxByte(){

int nTemp =0;
int i,j;
   I2C_Set_sda_H();
  
    SDA_select_in;
  
   _NOP();
   _NOP();
   _NOP();
   _NOP();
   
   for(i=0;i<8;i++){
     I2C_Set_sck_H();
     if(P1IN & SDA){
         nTemp |= (0x01 << (7-i));
     }
     for(j=30;j>0;j--);
     I2C_Set_sck_L();
     for(j=30;j>0;j--);
   }
   return nTemp;
   
}



int jj=0;


int I2C_GetACK(){
  int nTemp = 0;
 // int j;
  _NOP();
  _NOP();

  I2C_Set_sck_L();
  for(jj=5;jj>0;jj--);
  SDA_select_in;
  I2C_Set_sck_H();
  
  for(jj=5;jj>0;jj--);

  nTemp = (int)(P1IN & SDA);
    I2C_Set_sck_L();
    return(nTemp & SDA);
}


int I2C_Read(char *pBuf,char address){

  int nTemp = 0;

  I2C_START();
  
  I2C_TxHtoL(0xd0);
  nTemp = I2C_GetACK();

  I2C_TxHtoL(address);
  nTemp = I2C_GetACK();
  
  
   I2C_START();
  
  I2C_TxHtoL(0xd1);
  nTemp = I2C_GetACK();

  pBuf[address] = I2C_RxByte();
  nTemp = I2C_GetACK();  

  I2C_STOP();

  return nTemp;
}

int I2C_Write(char *pBuf,char address){

  int nTemp = 0;
  I2C_START();
  
  I2C_TxHtoL(0xd0);
  nTemp = I2C_GetACK();

  I2C_TxHtoL(address);
  nTemp = I2C_GetACK();
 
  I2C_TxHtoL(pBuf[address]);
  nTemp = I2C_GetACK();

  I2C_STOP();
  
  return nTemp;
  
}

void I2C_TxLtoH(int nValue){
  int i,j;
  for( i=0; i<8;i++){
    if(nValue &0x01)
      I2C_Set_sda_H();
    else
      I2C_Set_sda_L();
    
    for(j =30;j>0;j--);
    I2C_Set_sck_H();
    nValue >>=1;
    for(j =30;j>0;j--);
    I2C_Set_sck_L();
  }
    return;  
}

///-------------------------------------------------------------------//////

void bq32000_Init(void)
{

  P1DIR &= ~IRQ;
  /* P1IFG = 0;
  P1IES |= (IRQ);                                //選擇中斷模式(1 負緣觸發)
  P1IE  |= (IRQ);                                //沒使用到 所以暫時不接
*/
  
   SCL_select_out;
   SDA_select_out;
   
  I2C_Set_sck_L();
  I2C_STOP();
  Delay_ms(10);
  return;


}


void SetTime(char *pClock)
{
  
    I2C_Write(pClock,0x00);     //---sec
    I2C_Write(pClock,0x01);     //---sec
    I2C_Write(pClock,0x02);     //---sec
    I2C_Write(pClock,0x03);     //---sec
    I2C_Write(pClock,0x04);     //---sec
    I2C_Write(pClock,0x05);     //---sec
    I2C_Write(pClock,0x06);     //---sec
   
}

void GetTime(char *pTime)
{
   
     I2C_Read(pTime,0x00);
     I2C_Read(pTime,0x01);
     I2C_Read(pTime,0x02);
     I2C_Read(pTime,0x03);
     I2C_Read(pTime,0x04);
     I2C_Read(pTime,0x05);     
     I2C_Read(pTime,0x06);     
}

void OutPutMode(int mode){

  if(mode == Out_1Hz){

    I2C_Write_Char(0x40,0x07);    //啟動輸出訊號
    I2C_Write_Char(0x5E,0x20);    //SF KEY1
    I2C_Write_Char(0xC7,0x21);    //SF KEY2
    I2C_Write_Char(0x01,0x22);    //輸出模式
    
  }
  else if(mode == Out_512Hz){
    I2C_Write_Char(0x40,0x07);    //啟動輸出訊號
    I2C_Write_Char(0x5E,0x20);    //SF KEY1
    I2C_Write_Char(0xC7,0x21);    //SF KEY2
    I2C_Write_Char(0x00,0x22);    //輸出模式
  }

}

int I2C_Write_Char(int pBuf,char address){

  
  int nTemp = 0;
  I2C_START();
  
  I2C_TxHtoL(0xd0);
  nTemp = I2C_GetACK();

  I2C_TxHtoL(address);
  nTemp = I2C_GetACK();
 
  I2C_TxHtoL(pBuf);
  nTemp = I2C_GetACK();

  I2C_STOP();
  return nTemp;

}
//--------------------------------------------------------------------------////