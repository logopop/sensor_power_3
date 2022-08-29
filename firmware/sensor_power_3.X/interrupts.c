//*********************************************************************************************************************
//  OBDgauge sensor board power controller v2
//  Device 18f16q41 
//  
//    
//*********************************************************************************************************************
//
  


//*********************************************************************************************************************
//

#include <xc.h>
#include "sensor_power.h"
 
// MVECEN = OFF and IPEN = 1
void __interrupt() highPriorityInterrupt010(void)
{
   int   i = 0;
   int   j = 0;
    
   
   TMR0H = 0x3d;    // 15 sec
   TMR0L = 0x00;
   
   if (IOCCF0)
   {   
      
      ucStartflag = F_RING;   
     // ucProlong = 1;
     
   }   
   else if (IOCBF4)
      ucStartflag = F_IGNITION;
   else if (IOCBF5)
      ucStartflag = F_ALARM;
   else   
   {   
      ulLife++;
      if (!(ulLife % (SLEEP_PERIOD * 60 * 4)))  // 5760 = 24 hr
      {         
         ucStartflag = F_TIMER;
      }   
      else  // No timeout, just blink
      {
         LED_WHITE = 1;
         __delay_ms(30);
         LED_WHITE = 0;    
         
       
      }   
   }
   
   
   if (!ucStartflag)
   {   

      if (!(ulLife % 2))
         vGsm_on(); 
     // else
     //   vGsm_off(); 

   }   
    
   
   //*********
   
   IOCCF0 = 0;
   IOCBF4 = 0;
   IOCBF5 = 0;
   TMR0IF = 0;
   
   
}
 






