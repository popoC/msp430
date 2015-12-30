//******************************************************************************
// P1
//   -P1.0(I/O)Out          LED         //- 
//   -P1.1(I/O)Interrupt    Sea_125Hz   //- 
//   -P1.2(I/O)Interrupt    Sea_1pps    //- P14-1
//   -P1.3(I/O)Out          pps_out     //- P14-3
//   -P1.4(I/O)
//   -P1.5(I/O)Interrupt    GPS_1PPS     //- P14-5
//   -P1.6(I/O)x
//   -P1.7(I/O)x

// P2
//   -P2.0(I/O)Out         ADS1222_SCLK
//   -P2.1(I/O)Interrupt   ADS1222_D1
//   -P2.2(I/O)Interrupt   ADS1222_D2
//   -P2.3(I/O)Interrupt   ADS1222_D3
//   -P2.4(I/O)Interrupt   ADS1222_D4
//   -P2.5(I/O)x
//   -P2.6(I/O)x
//   -P2.7(I/O)x

// P3
//   -P3.0(I/O)Out         SD_EN
//   -P3.1(I/O)Interrupt   SD_SIMO
//   -P3.2(I/O)Interrupt   SD_SOMI
//   -P3.3(I/O)Out         SD_SCLK
//   -P3.4(I/O)Interrupt   UR_1_TX
//   -P3.5(I/O)Interrupt   UR_1_RX
//   -P3.6(I/O)x
//   -P3.7(I/O)x

// P4
//   -P4.0(I/O)x
//   -P4.1(I/O)Out         ADS1222_CLK 
//   -P4.2(I/O)x 
//   -P4.3(I/O)x
//   -P4.4(I/O)x
//   -P4.5(I/O)x
//   -P4.6(I/O)x
//   -P4.7(I/O)x

// P5
//   -P5.0(I/O)x
//   -P5.1(I/O)x 
//   -P5.2(I/O)x 
//   -P5.3(I/O)x
//   -P5.4(I/O)x
//   -P5.5(I/O)x
//   -P5.6(I/O)Interrupt   UR_2_TX
//   -P5.7(I/O)Interrupt   UR_2_RX

// P6
//   -P6.0(I/O)x
//   -P6.1(I/O)x 
//   -P6.2(I/O)x 
//   -P6.3(I/O)x
//   -P6.4(I/O)Out         RelayOn
//   -P6.5(I/O)Out         RleayOff  
//   -P6.6(I/O)x
//   -P6.7(I/O)x

// P7
//   -P7.0(I/O)x           Xin
//   -P7.1(I/O)x           Xout
//   -P7.2(I/O)x 
//   -P7.3(I/O)x
//   -P7.4(I/O)x
//   -P7.5(I/O)x
//   -P7.6(I/O)x
//   -P7.7(I/O)x

// P8 all x
// P9 
//   -P9.0(I/O)x
//   -P9.1(I/O)x 
//   -P9.2(I/O)x 
//   -P9.3(I/O)x
//   -P9.4(I/O)Interrupt   UR_3_TX
//   -P9.5(I/O)Interrupt   UR_3_RX
//   -P9.6(I/O)x
//   -P9.7(I/O)x

//******************************************************************************



#ifndef MAIN_H
#define MAIN_H



//#include "sdcomm_spi.h"

/* Peripheral clock rate, in Hz, used for timing. */
#define PERIPH_CLOCKRATE 20000000
#define LOGGER_TIME 1                                      // 輸出時間   -> PC 
#define AD_DATA 2                                          // 輸出AD     -> PC
#define SDCARD_CAPACITY 3                                  // 輸出SD容量 -> PC
#define SEASCAN_CONNECT 4                                  // 連接SEASCAN
#define SE_GPS 5                                           // 比較GPS 1PPS

#define open_relay_test 6
#define close_relay_test 7

#define GET_SEASTR 8

#define RE_SET 66
#define Channel_N 4                                        //使用通道數目

//int do_sd_initialize (sd_context_t *sdc);

#endif
