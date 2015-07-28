/*******************************************************************************
* File Name: main.c
*
* Version: 2.10
*
* Description:
*  The PWM datasheet example project.
*  This example project demonstrates 8 bit PWM Fixed Function Block.
*
********************************************************************************
* Copyright 2012, Cypress Semiconductor Corporation. All rights reserved.
* This software is owned by Cypress Semiconductor Corporation and is protected
* by and subject to worldwide patent and copyright laws and treaties.
* Therefore, you may use this software only as provided in the license agreement
* accompanying the software package from which you obtained this software.
* CYPRESS AND ITS SUPPLIERS MAKE NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* WITH REGARD TO THIS SOFTWARE, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT,
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*******************************************************************************/

#include <project.h>

/*******************************************************************************
* Function Name: main
********************************************************************************
*
* Summary:
*  Main function performs following functions:
*   1: Enables the clock
*   2: Initializes the LCD
*   3: Start the PWM
*   4: Write a byte to control register to set Kill mode
*   5: Read the current value assigned to a Control Register 
*   6: Read the current value assigned to Status Register
*   7: Print Period value in LCD
*   8: Print Compare value in LCD
*   9: Print current status of PWM output in LCD
*
* Parameters:
*  None.
*
* Return:
*  None.
*
*******************************************************************************/
uint8 mycounter = 0;
uint8 bufsize = 0;
uint8 numSensors = 11;
uint8 i = 0;
uint16 birlBreathData[2];
uint16 birlBreathAbs;
uint8 firstBreath = 0;

uint16 breathBaseline;
uint16 maxBreathPos;
uint16 threshold;
uint16 maxBreathNeg;
uint16 birlBreathPos;
uint16 birlBreathNeg;
uint8 breathLED;
uint8 breathMIDI;

uint8 xy1_address = 8;
uint8 xy2_address = 9;
uint8 xy1[2] = {128,128};
uint8 xy2[2] = {128,128};
uint8_t teensy_address = 4;
uint8 teensy_send[2] = {64,64};
uint8 j = 0;

void myLEDFunction(uint8);

void sendKeys(uint8 value, uint8 which);

void sendBreath(void);

void scanBreathSensor(void);

void sendLEDBreath(void);

void sendXY(uint8 value, uint8 which);

void sendI2CTest(uint8 value);

int main()
{
    /* Prepare components */
    Clock_1_Enable();    
    PWM_1_Start();    
    Clock_2_Enable();   
    Clock_2_Start();   
    CapSense_Start();
	SPIM_Start();

    /* Enable global interrupts */
    CyGlobalIntEnable;
    
    UART1_Start();     /* Enabling the UART */
    I2C_Init();
    I2C_Start();
    
    I2C_1_Init();
    I2C_1_Start();
    
    /* Initialize baselines */ 
    CapSense_InitializeAllBaselines();
    
    while(1u)
    {
        sendI2CTest(j);
        j++;
        for (i = 0; i < numSensors; i++)
        {
            /* Update all baselines */
            CapSense_UpdateEnabledBaselines();
        
   		    /* Start scanning all enabled sensors */
    	    CapSense_ScanSensor(i);     
    
            /* Wait for scanning to complete */
		
            while(CapSense_IsBusy() != 0)
            {
                ;
            }

            sendKeys(((uint8)(CapSense_sensorSignal[i] / 8)), i);

            scanBreathSensor();
            sendBreath();
            
            if (XY1_DATAREADY_Read())
            {
                
                I2C_MasterReadBuf(xy1_address, xy1, 2, I2C_MODE_COMPLETE_XFER);

                while (0u == (I2C_MasterStatus() & I2C_MSTAT_RD_CMPLT))
                {
                    ;
                }
                   
                    
                I2C_MasterClearStatus();
                
                sendXY((xy1[0] / 2), 0);
                sendXY((xy1[1] / 2), 1);
            }
            
            if (XY2_DATAREADY_Read())
            {
                
                I2C_MasterReadBuf(xy2_address, xy2, 2, I2C_MODE_COMPLETE_XFER);

                while (0u == (I2C_MasterStatus() & I2C_MSTAT_RD_CMPLT))
                {
                    ;
                }
                   
                    
                I2C_MasterClearStatus();
                
                sendXY((xy2[0] / 2), 2);
                sendXY((xy2[1] / 2), 3);
            }
        }
    }

    return 0;
}

void myLEDFunction(uint8 inputValue)
{
    PWM_1_WriteCompare(inputValue);
}

void sendI2CTest(uint8 value)
{
    teensy_send[0] = value;
    I2C_1_MasterWriteBuf(teensy_address, teensy_send, 1, I2C_1_MODE_COMPLETE_XFER);
    while (0u == (I2C_1_MasterStatus() & I2C_1_MSTAT_RD_CMPLT))
    {
        ;
    }                       
    I2C_1_MasterClearStatus();
}

void sendKeys(uint8 value, uint8 which)
{
    UART1_PutChar(176); /* Sending the data */
    //CyDelay(10);
    UART1_PutChar(20 + which); /* Sending the data */
    //CyDelay(10);
    UART1_PutChar(value); /* Sending the data */
}

void sendXY(uint8 value, uint8 which)
{
    UART1_PutChar(176); /* Sending the data */
    //CyDelay(10);
    UART1_PutChar(31 + which); /* Sending the data */
    //CyDelay(10);
    UART1_PutChar(value); /* Sending the data */
}

void scanBreathSensor(void)
 {
    SPIM_WriteByte(0);
    birlBreathData[0] = SPIM_ReadRxData();
    SPIM_WriteByte(0);
    birlBreathData[1] = SPIM_ReadRxData();
	
	birlBreathAbs = ((birlBreathData[0] << 4) | (birlBreathData[1] >> 12));
	// check for first breath, if so, take in ambient pressure reading, if not first breath, subtract baseline from breath reading in sendBirlBreathOSC
	
	if (firstBreath == 0)
		firstBreath++;
	else if (firstBreath == 1) {
		breathBaseline = birlBreathAbs;
		maxBreathPos = 65535 - breathBaseline + threshold;
		maxBreathNeg = breathBaseline - threshold;
		firstBreath = 2;
	}
}


void sendLEDBreath(void) {
	if ((birlBreathNeg == 0) && (birlBreathPos == 0))
    {
        myLEDFunction(0);	
    }
	else if (birlBreathPos > 0) 
    {
		breathLED  = (uint8)((birlBreathPos*255) / maxBreathPos / 2);
        myLEDFunction(breathLED);
	}
	else if (birlBreathNeg > 0) 
    {
		;
	}
}


void sendBreath(void)
{

	
	if (birlBreathAbs < (breathBaseline - threshold))
	{
		birlBreathNeg = (breathBaseline - threshold) - birlBreathAbs;
		birlBreathPos = 0;
	}
	else if (birlBreathAbs > (breathBaseline + threshold))
	{
		birlBreathPos = birlBreathAbs - (breathBaseline + threshold);
		birlBreathNeg = 0; 
	}
	else 
	{
		birlBreathNeg = 0;
		birlBreathPos = 0;	
	}
    breathMIDI = birlBreathPos / 512;
    
     UART1_PutChar(176); /* Sending the data */ // CC messsage
    //CyDelay(10);
    UART1_PutChar(2); /* Sending the data */ //CC2 = breath controller
    //CyDelay(10);
    UART1_PutChar(breathMIDI); /* Sending the data */ // the value
    
    //now make the LED reflect the breath value
    sendLEDBreath();
}
/* [] END OF FILE */
