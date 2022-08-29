//*********************************************************************************************************************
//  OBDgauge sensor board power controller v3
//  Hardware version 5
//  Device 18f16q41 
//  
//    
//*********************************************************************************************************************
//
  


//*********************************************************************************************************************
//
 
#include <xc.h>
#include "sensor_power.h"
 
//*********************************************************************************************************************
// 
// 
//*********************************************************************************************************************
//

void vGsm_on(void)
{
   
   GSM_PWR = 0;

}   

void vGsm_off(void)
{

   GSM_PWR = 1;
   
}   

//*********************************************************************************************************************
// 
// 
//*********************************************************************************************************************
//

void vBlink(uint8_t ucNum, uint8_t ucColor)
{
   uint8_t                       i                          = 0;

   for (i = 0; i < ucNum; i++)
   {         
      if (ucColor == WHITE)
         LED_WHITE = 1;
      else if (ucColor == PINK)
         LED_PINK = 1;

      __delay_ms(10);
      
      if (ucColor == WHITE)
         LED_WHITE = 0;
      else if (ucColor == PINK)
         LED_PINK = 0;
      __delay_ms(200);
           
   }
}   
 
//*********************************************************************************************************************
// 
// 
//*********************************************************************************************************************
//
  
uint8_t ucInitialize(void)
{
   uint8_t                       i                          = 0;
   
   OSCENbits.ADOEN = 0;
   OSCENbits.PLLEN = 0;
   OSCENbits.HFOEN = 0;
   OSCENbits.EXTOEN = 0;
   OSCENbits.MFOEN = 0;
   OSCENbits.SOSCEN = 0;
   OSCCON3bits.SOSCPWR = 0;
  // CPUDOZEbits.IDLEN = 1;        // Sleep is idle

   // Ports 
   ANSELA = 0x00;
   ANSELB = 0x00;
   ANSELC = 0x00;
   TRISA  = 0x00;
   TRISB  = 0x00;
   TRISC  = 0x00;
         
   // Set tris, 1 is input
   TRISCbits.TRISC0 = 1;                  // !RING
   TRISCbits.TRISC1 = 0;                  // Pending power down (warning)
   TRISCbits.TRISC7 = 0;                  // White LED
   TRISCbits.TRISC6 = 0;                  // GSM power

   TRISBbits.TRISB4 = 1;                  // Ignition
   TRISBbits.TRISB5 = 1;                  // Alarm
   TRISBbits.TRISB7 = 1;                  // Shutdown signal from CPU

   TRISAbits.TRISA4 = 1;                  // Future use, to CPU, sensors v5
   
   INLVLC = 0;
     
   
   // Set Open Drain for GSM power
   ODCONCbits.ODCC6 = 1;
    
   // Init port states
   POWER = 0;
   LED_WHITE = 0;
   LED_PINK = 0;
   PWR_DWN = 0;
   GSM_PWR = 1;                           // 1 is open, 0 is pulldown
   
   CPUDOZEbits.IDLEN = 0;                 // Allow for sleep
  
   // Timer 0 - wakeup   
   T0CON0bits.MD16 = 1;                   // 16 bit mode
   T0CON1bits.CS = 0b100;                 // Clock source
   T0CON1bits.ASYNC = 0;                  // Async mode, run in sleep
   T0CON1bits.CKPS = 0b0001;              // Prescaler  
   T0CON0bits.OUTPS = 0b0001;             // Postscaler
   TMR0H = 0xf0;                          // Start values
   TMR0L = 0xf0;
   T0CON0bits.EN = 1; 
       
   // Interrupts
   INTCON0bits.IPEN = 0;
   INTCON0bits.GIE = 1;
   INTCON0bits.GIEL = 1;
   PIE0bits.SWIE = 1;
   PIE3bits.TMR0IE = 1;                   // Timer 0 int enable   
   PIE0bits.IOCIE = 1;                    // Interrupt on change
   
   // Interrupt on change
   IOCCP0 = 1;                            // !RING     
   IOCBN4 = 1;                            // Ignition
   IOCBN5 = 1;                            // Alarm
   
   // Start signal
   vBlink(10, WHITE);
    
   
   
   return 0;
}   

//*********************************************************************************************************************
// 
// 
//*********************************************************************************************************************
//
 

//*********************************************************************************************************************
// 
// 
//*********************************************************************************************************************
//
 
uint8_t ucMain(void)
{
   unsigned long                 i                          = 0;
   unsigned long                 j                          = 0;
   
  
   // Main sleeploop
   while (1)
   {                        
      if (!IGNITION)                         // Capture high on start
      {   
         ucBusy = 1; 
         LED_PINK = 1;  
         ucStartflag = F_IGNITION;
         vGsm_on();
      }   
             
      // There is an interrupt or timeout
      if (ucStartflag)
      {   
         vGsm_on();
    
         if (ucStartflag == F_TIMER)
            vBlink(2, WHITE);
         else if (ucStartflag == F_IGNITION)
         {   
            vBlink(3, WHITE);                           
         }   
         else if (ucStartflag == F_ALARM)
         {
            vBlink(4, WHITE);
            PWR_DWN = 1;                     // Signal alarm triggered
         }   
         else if (ucStartflag == F_RING)
            vBlink(5, WHITE);
             
         POWER = 1;                          // Fire up CPU
         __delay_ms(1500);
          
         ulLife = 0;                         // Reset 15 second counter
         ucBusy = 1;
         
         //*************************************************************
         while (ucBusy)                      // In service
         {
            vBlink(1, WHITE);
            if (IGNITION)                    // No ignition signal
               LED_PINK = 0;
            else
               LED_PINK = 1;
                            
            // Reset time if incoming msg/ring and not about to shut down
            if ((IGNITION) && (!PWR_DWN))    
            {
               if (ucProlong)
               {
                  ucProlong = 0;
                  ulLife = 0;                // Reset timer counter again                                 
               }
            }
            
            // Signal pending shutdown
            if (ulLife >= ((OPERATE_TIME * 4) - 1))   // 15 second warning
            {
               if (IGNITION)
               {
                  PWR_DWN = 1;
                  vBlink(1, PINK);                
               }
            }   
            
            // Check exit conditions
            if (ulLife >= OPERATE_TIME * 4)  // Hard timeout after x seconds if ignition is off
            {   
               if (IGNITION)                 // High when off                
                  ucBusy = 0;  
               else
                  ulLife = 0;                // Reset to stay on a period after ignition is turned off
            }   
            if (SHUTDOWN)                    // CPU is done, shut down if ignition not on
            {   
               if (IGNITION)                 // High when off               
                  ucBusy = 0;                                                                
            }
            if (!ucBusy)
               ulLife = 0;
         }
         PWR_DWN = 0;
      }       
      // Done handling event 
  
      POWER = 0;
     
      ucStartflag = 0;
      //SLEEP();
   }
   return 0;
} 
 
//*********************************************************************************************************************
// 
// 
//*********************************************************************************************************************
//

void main() 
{            
   // Initialize device 
   ucInitialize();
  
   // Run routine
   ucMain();   
}

//*********************************************************************************************************************
// 
// 
//*********************************************************************************************************************
//


