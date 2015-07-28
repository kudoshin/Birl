/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include <project.h>

#define BUFFER_SIZE 4

uint16 pos[BUFFER_SIZE-1];
uint8 touched = 0;
uint8 ezi2cBuffer[BUFFER_SIZE];
uint8 i = 0;

int main()
{
    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    EZI2C_Init();
    EZI2C_Start();
    CapSense_Start();
    CyGlobalIntEnable;
    CapSense_InitializeAllBaselines();
    EZI2C_EzI2CSetBuffer1(BUFFER_SIZE, BUFFER_SIZE, ezi2cBuffer);
    /* CyGlobalIntEnable; */ /* Uncomment this line to enable global interrupts. */
    for(;;)
    {
           /* Update all baselines */
            CapSense_UpdateEnabledBaselines();
        
   		    /* Start scanning all enabled sensors */
    	    CapSense_ScanEnabledWidgets();   
    
            /* Wait for scanning to complete */
		
            while(CapSense_IsBusy() != 0)
            {
                ; //wait for the scan to finish
            }
            
            touched = CapSense_GetTouchCentroidPos(CapSense_TOUCHPAD0__TP, pos);
            
            
            ezi2cBuffer[0] = (uint8) pos[0];
            ezi2cBuffer[1] = (uint8) pos[1];
            ezi2cBuffer[2] = touched;
            
            //ezi2cBuffer[0] = i;
            //ezi2cBuffer[1] = i+1;
            //i++;
            XY_DATAREADY_Write(1);
            
            while (!(EZI2C_EzI2CGetActivity() & EZI2C_EZI2C_STATUS_READ1))
            {
                ; //wait for the data to be read from the master
            }
            
            XY_DATAREADY_Write(0);
            
    }
}

/* [] END OF FILE */
