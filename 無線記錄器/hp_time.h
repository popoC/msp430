#ifndef HP_TIME_H
#define HP_TIME_H
#include "msp430x54xA.h"

typedef signed long long hptime_t;

typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef signed short int16_t;



/* Error code for routines that normally return a high precision time.
 * The time value corresponds to '1902/1/1 00:00:00.000000' with the
 * default HPTMODULUS */
#define HPTERROR -2145916800000000LL
//static hptime_t HPTERROR = -2145916800000000LL;
/* Default modulus of 1000000 defines tick interval as a microsecond */
#define HPTMODULUS 1000000


/* SEED binary time */
typedef struct btime_s
{
  uint16_t  year;
  uint16_t  day;
  uint8_t   hour;
  uint8_t   min;
  uint8_t   sec;
  uint8_t   unused;
  uint16_t  fract;
}
BTime;

//void RS232_Send_Char(char *str,int count,BYTE com);

hptime_t ms_timestr2hptime (char *timestr);
int  ms_md2doy (int year, int month, int mday, int *jday);
static hptime_t ms_time2hptime_int (int year, int day, int hour, int min, int sec, int usec);
hptime_t ms_btime2hptime (BTime *btime);
#endif