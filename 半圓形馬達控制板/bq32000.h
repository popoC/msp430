//#define CE	BIT0
//#define SDA	BIT1
//#define SCLK    BIT2

//#define SCL BIT1
#define SCL BIT1      
#define SDA BIT2
#define IRQ BIT0
#define Out_1Hz   1
#define Out_512Hz 512


#define SDA_select_out P1DIR|=SDA
#define SDA_select_in  P1DIR&=(~SDA)

#define SDA_out_H  P1OUT|=SDA
#define SDA_out_L  P1OUT&= ~(SDA)

#define SCL_select_out P1DIR|=SCL
#define SCL_select_in  P1DIR&=(~SCL)

#define SCL_out_H  P1OUT|=SCL
#define SCL_out_L  P1OUT&= ~(SCL)




void I2C_Set_sda_H();
void I2C_Set_sda_L();
void I2C_Set_sck_H();
void I2C_Set_sck_L();
void I2C_START();
void I2C_STOP();
void I2C_TxHtoL(int nValue);
void I2C_TxLtoH(int nValue);
int I2C_RxByte();
int I2C_GetACK();
void I2C_SetACK();

void I2C_init();
int I2C_Read(char *pBuf,char address);
int I2C_Write(char *pBuf,char address);

void Delay_ms(unsigned long nValue);

void SetTime(char *pClock);
void GetTime(char pTime[]);
void bq32000_Init(void);

int I2C_Write_Char(int pBuf,char address);
void OutPutMode(int mode);
