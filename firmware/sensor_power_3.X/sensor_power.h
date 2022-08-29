//*********************************************************************************************************************
//  OBDgauge sensor board power controller
//   
//  
//    
//*********************************************************************************************************************
//
 
// PIC18F16Q41 Configuration Bit Settings

// CONFIG1
#pragma config RSTOSC = LFINTOSC //HFINTOSC_1MHZ
#pragma config FEXTOSC = OFF    //  

#pragma LFINTOSC//HFINTOSC_1MHZ           // Reset Oscillator Selection (HFINTOSC with HFFRQ = 4 MHz and CDIV = 4:1)

// CONFIG2
#pragma config CLKOUTEN = OFF   // Clock out Enable bit (CLKOUT function is disabled)
#pragma config PR1WAY = ON      // PRLOCKED One-Way Set Enable bit (PRLOCKED bit can be cleared and set only once)
#pragma config CSWEN = OFF       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor enabled)
#pragma config FCMENP = ON      // Fail-Safe Clock Monitor - Primary XTAL Enable bit (Fail-Safe Clock Monitor enabled; timer will flag FSCMP bit and OSFIF interrupt on EXTOSC failure.)
#pragma config FCMENS = ON      // Fail-Safe Clock Monitor - Secondary XTAL Enable bit (Fail-Safe Clock Monitor enabled; timer will flag FSCMP bit and OSFIF interrupt on SOSC failure.)
 

// CONFIG3
#pragma config MCLRE = EXTMCLR  // MCLR Enable bit (If LVP = 0, MCLR pin is MCLR; If LVP = 1, RE3 pin function is MCLR )
#pragma config PWRTS = PWRT_OFF // Power-up timer selection bits (PWRT is disabled)
#pragma config MVECEN = OFF      // Multi-vector enable bit (Multi-vector enabled, Vector table used for interrupts)
#pragma config IVT1WAY = ON     // IVTLOCK bit One-way set enable bit (IVTLOCKED bit can be cleared and set only once)
#pragma config LPBOREN = OFF    // Low Power BOR Enable bit (Low-Power BOR disabled)
#pragma config BOREN = SBORDIS  // Brown-out Reset Enable bits (Brown-out Reset enabled , SBOREN bit is ignored)

// CONFIG4
#pragma config BORV = VBOR_1P9  // Brown-out Reset Voltage Selection bits (Brown-out Reset Voltage (VBOR) set to 1.9V)
#pragma config ZCD = OFF        // ZCD Disable bit (ZCD module is disabled. ZCD can be enabled by setting the ZCDSEN bit of ZCDCON)
#pragma config PPS1WAY = ON     // PPSLOCK bit One-Way Set Enable bit (PPSLOCKED bit can be cleared and set only once; PPS registers remain locked after one clear/set cycle)
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config LVP = ON         // Low Voltage Programming Enable bit (Low voltage programming enabled. MCLR/VPP pin function is MCLR. MCLRE configuration bit is ignored)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Extended Instruction Set and Indexed Addressing Mode disabled)

// CONFIG5
#pragma config WDTCPS = WDTCPS_31// WDT Period selection bits (Divider ratio 1:65536; software control of WDTPS)
#pragma config WDTE = OFF        // WDT operating mode (WDT enabled regardless of sleep; SWDTEN is ignored)

// CONFIG6
#pragma config WDTCWS = WDTCWS_7// WDT Window Select bits (window always open (100%); software control; keyed access not required)
#pragma config WDTCCS = LFINTOSC      // WDT input clock selector (Software Control)

// CONFIG7
#pragma config BBSIZE = BBSIZE_512// Boot Block Size selection bits (Boot Block size is 512 words)
#pragma config BBEN = OFF       // Boot Block enable bit (Boot block disabled)
#pragma config SAFEN = OFF      // Storage Area Flash enable bit (SAF disabled)
#pragma config DEBUG = OFF      // Background Debugger (Background Debugger disabled)

// CONFIG8
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot Block not Write protected)
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers not Write protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not Write protected)
#pragma config WRTSAF = OFF     // SAF Write protection bit (SAF not Write Protected)
#pragma config WRTAPP = OFF     // Application Block write protection bit (Application Block not write protected)

// CONFIG9
#pragma config CP = OFF         // PFM and Data EEPROM Code Protection bit (PFM and Data EEPROM code protection disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.
 
//*********************************************************************************************************************
//
// Includes
//

#include       <ctype.h>
#include       <string.h>
#include       <stdio.h>
#include       <stdarg.h>
#include       <math.h>
#include       <float.h>
 
//*********************************************************************************************************************
//
// Bit Defines
//


/*
#define IGNITION        0b00100000        // In:  Ignition optocouple
#define ALARM           0b00010000        // In:  Alarm optocouple
#define CALL            0b00001000        // In:  Call/message signal from GSM
#define SHUTOFF         0b00000100        // In:  Shutoff signal from CPU 
#define LED             0b00000010        // Out: To indicator LED
#define RELAY           0b00000001        // Out: To power relay
*/

void vGsm_on(void);
void vGsm_off(void);

//*********************************************************************************************************************
//
// Program defines
//

#define _XTAL_FREQ      31000  //1000000
 

#define SLEEP_PERIOD    24                // 24 hours between wakeups when not triggered
#define OPERATE_TIME    2                 // minutes on when not shut down by cpu
  
// Start flag defines
enum cFlag
{
   F_DUMMY,
   F_IGNITION,
   F_ALARM,
   F_RING,
   F_TIMER
};   

//*********************************************************************************************************************
//
// Port defines
//

// Outputs
#define                          LED_PINK                   LATAbits.LATA2    // Pink board LED             (17)
#define                          LED_WHITE                  LATCbits.LATC7    // White board LED            (9)
#define                          GSM_PWR                    LATCbits.LATC6    // GSM module power           (8)
#define                          POWER                      LATBbits.LATB6    // Main power on              (11)

// LED colors
#define                          WHITE                      1                 // White    
#define                          PINK                       2                 // Pink

// Inputs
#define                          RING                       PORTCbits.RC0     // Ring signal from GSM       (16)
#define                          PWR_DWN                    LATCbits.LATC1    // To CPU, pending power down (15)

#define                          IGNITION                   PORTBbits.RB4     // Low when ignition is on    (13)
#define                          ALARM                      PORTBbits.RB5     // Low when alarm triggered   (12)
#define                          SHUTDOWN                   PORTBbits.RB7     // From CPU, shutdown signal  (10)

#define                          NOT_USED                   PORTAbits.RA4     // Future use sensor board v5 (3)    

// Globals
unsigned long                    ulLife                     = 0;
uint8_t                          ucStartflag                = 0;         
uint8_t                          ucBusy                     = 0;
uint8_t                          ucProlong                  = 0;              // Extend running time due to ring/msg

//*********************************************************************************************************************
//

void vBlink(uint8_t, uint8_t);



















