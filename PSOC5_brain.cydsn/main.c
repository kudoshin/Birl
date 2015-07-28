/*******************************************************************************
* File Name: main.c

This is the brain for the MiniBirl - designed to run on a Cypress PSOC5 microcontroller.
*******************************************************************************/

#include <project.h>

/*******************************************************************************
* Function Name: main
********************************************************************************
*
* Summary:
*  Main function performs following functions:
*   1: sets up the clocks and enables the different modules
    2: starts scanning the capacitive key sensors
    3: scans the breath sensor and sends it over I2C
    4: checks whether there is new XY touch data ready and if so, collects and sends it
    5: waits for the capacitive key sensor scan to finish if it's not yet done (about 3 ms for full scan of 11 keys)
    6: sends capacitive key sensor data
    7: GOTO INFINITY 4-EVA
*
* Parameters:
*  None.
*
* Return:
*  None.
*
*******************************************************************************/
#define NUM_SENSORS 11

uint8 mycounter = 0;
uint8 bufsize = 0;
uint8 i = 0;
uint16 birlBreathData[2];
uint16 birlBreathAbs;
uint16 firstBreath = 0;

uint16 breathBaseline;
uint16 maxBreathPos;
uint16 threshold;
uint16 maxBreathNeg;
uint16 birlBreathPos;
uint16 birlBreathNeg;
uint16 prevBirlBreathPos = 65000; // initialize with nonsense so that the test for new data comes out true on the first round of sensing
uint16 prevBirlBreathNeg = 65000; // initialize with nonsense
uint8 breathLED;
uint8 breathMIDI;

uint8 xy1_address = 8;
uint8 xy2_address = 9;
uint8 xy1[3] = {64,64,0};
uint8 xy2[3] = {64,64,0};
uint8 xy1_prev[3] = {127,127,2}; // initialize with nonsense
uint8 xy2_prev[3] = {127,127,2}; // initialize with nonsense
uint8_t teensy_address = 0;
uint8 teensy_send[4];
uint8 j = 0;
uint16 timeout = 0;
uint8 keysoffset = 6;
uint8 xyoffset = 2;
uint8 touchoffset = 17;
uint8 buttonoffset = 19;

uint8 buttons;
uint8 prevButtons = 0x4;  // initialize with nonsense

uint16_t current_data[NUM_SENSORS];

void myLEDFunction(uint8);

void sendKeys(uint16 value, uint8 which);

void sendBreath(void);

void scanBreathSensor(void);

void sendLEDBreath(void);

void sendXY(uint16 value, uint8 which);

void sendtouch(uint8 value, uint8 which);

void sendI2CMidiData(void);

void processXYtouchpads();

void sendButtons();

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
    
    PWM_1_WriteCompare(0);
    Pressure_CS_Write(1);
    CyDelay(100);
    //UART1_Start();     /* Enabling the UART */
    // I2C_Init(); calles in start
    I2C_Start();
    CyDelay(100);
    // I2C_1_Init();
    I2C_1_Start();
    CyDelay(100);
    /* Initialize baselines */ 
    CapSense_InitializeAllBaselines();
    CyDelay(100);
    for (i = 0; i < NUM_SENSORS; i++)
    {
        current_data[i] = 12345;
    }
    
    while(1u)
    {
        
        // Update all baselines 
        CapSense_UpdateEnabledBaselines();
    
	    // Start scanning all enabled sensors 
	    CapSense_ScanEnabledWidgets();     

        //scan and send breath data
        scanBreathSensor();
        sendBreath();
        
        //check if the XY pads have data, and if so, collect and send it
        processXYtouchpads();
        
        buttons = (~(Switch_up_Read() | (Switch_down_Read() << 1))) & 0x3;
        sendButtons();
        
        // Wait for key scanning to complete
        while(CapSense_IsBusy() != 0)
        {
            ;
        }
        
        //send key data to computer
        for (i = 0; i < NUM_SENSORS; i++)
        {
            if (current_data[i] != CapSense_sensorSignal[i])
            {
                sendKeys((CapSense_sensorSignal[i]), i);
                current_data[i] = CapSense_sensorSignal[i];
            }
        }
    }

    return 0;
}

void myLEDFunction(uint8 inputValue)
{
    PWM_1_WriteCompare(inputValue);
}

void sendI2CMidiData(void)
{
    I2C_1_MasterWriteBuf(teensy_address, teensy_send, 4, I2C_1_MODE_COMPLETE_XFER);
    timeout = 1000;
    while ((0u == (I2C_1_MasterStatus() & I2C_1_MSTAT_RD_CMPLT)) && (timeout > 0))
    {
        timeout--;
    }                       
    I2C_1_MasterClearStatus();

}

void sendKeys(uint16 value, uint8 which)
{
    teensy_send[0] = which + keysoffset;
    teensy_send[1] = (value >> 7);
    teensy_send[2] = (value & 127);
    sendI2CMidiData();
}

void sendXY(uint16 value, uint8 which)
{
    teensy_send[0] = which + xyoffset;
    teensy_send[1] = (value >> 7);
    teensy_send[2] = (value & 127);
    sendI2CMidiData();
}

void sendtouch(uint8 value, uint8 which)
{
    teensy_send[0] = which + touchoffset;
    teensy_send[1] = 0;
    teensy_send[2] = (value & 1);
    sendI2CMidiData();
}

void scanBreathSensor(void)
 {
    Pressure_CS_Write(0); // set the pressure sensor ADC CS pin low to signal the ADC chip that you will be talking to it over SPI
    CyDelayUs(20); // can this little wait be reduced? Worth testing.
    SPIM_WriteByte(0); // do a "dummy write" to send SPI clocks to the ADC so that you can get data back from it
    birlBreathData[0] = SPIM_ReadRxData();
    SPIM_WriteByte(0);
    birlBreathData[1] = SPIM_ReadRxData();
	
	birlBreathAbs = ((birlBreathData[0] << 4) | (birlBreathData[1] >> 12)); //decode the data that came back. The 16 bits of the reading are spread across the middle of two 16-bit words, so that the first 4 bits can be zeroes while the conversion is executed
	// check for the 100th breath, if so, take in ambient pressure reading, if not 100th breath, subtract baseline from breath reading in sendBirlBreathOSC
	// why 100th? I had troubles with initial state stability when initializing ambient pressure from the first sensor reading. This could probably be much closer to the startup time, such as 10th rather than 100th reading.
    if (firstBreath < 100)
    {
        firstBreath++;
    }
	else if (firstBreath == 100) {
		breathBaseline = birlBreathAbs;
		maxBreathPos = 65535 - breathBaseline + threshold;
		maxBreathNeg = breathBaseline - threshold;
        firstBreath++;
	}
    Pressure_CS_Write(1); // put the CS pin for the pressure sensor ADC back into idle mode
}


void sendLEDBreath(void) {
	uint16 tempBreath = 0;
    if ((birlBreathNeg == 0) && (birlBreathPos == 0))
    {
        myLEDFunction(0);	
    }
	else if (birlBreathPos > 0)  
    {
        tempBreath = birlBreathPos;
        if (tempBreath > 32767)
        {
            tempBreath = 32767;
        }
        breathLED  = (uint8)(tempBreath / 128);
        myLEDFunction(breathLED);
	}
	else if (birlBreathNeg > 0) 
    {
	    tempBreath = birlBreathNeg;
        if (tempBreath > 32767)
        {
            tempBreath = 32767;
        }
        breathLED  = (uint8)(tempBreath / 128);
        myLEDFunction(breathLED);
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
    //convert to 14-bit
    birlBreathPos = birlBreathPos >> 2;
    birlBreathNeg = birlBreathNeg >> 2;
    
    //only send data if it's different from the last data sent
    if (birlBreathPos != prevBirlBreathPos)
    {
        
        teensy_send[0] = 0;
        teensy_send[1] = (birlBreathPos >> 7);
        teensy_send[2] = (birlBreathPos & 127);
        sendI2CMidiData();      
        prevBirlBreathPos = birlBreathPos;
    }
    else if (birlBreathNeg != prevBirlBreathNeg)
    {   
        teensy_send[0] = 1;
        
        teensy_send[1] = (birlBreathNeg >> 7);
        teensy_send[2] = (birlBreathNeg & 127);
        sendI2CMidiData();
        prevBirlBreathNeg = birlBreathNeg;
    }
    //now make the LED reflect the breath value
    sendLEDBreath();
}

void processXYtouchpads(void)
{      
    if (XY1_DATAREADY_Read())
    {
        
        I2C_MasterReadBuf(xy1_address, xy1, 3, I2C_MODE_COMPLETE_XFER);

        timeout = 1000;
        while ((0u == (I2C_MasterStatus() & I2C_MSTAT_RD_CMPLT))&& (timeout > 0))
        {
            timeout--;
        }
            
        I2C_MasterClearStatus();
        
        if (xy1[0] != xy1_prev[0])
        {
            sendXY((xy1[0]), 0);
            xy1_prev[0] = xy1[0];
        }
        if (xy1[1] != xy1_prev[1])
        {
            sendXY((xy1[1]), 1);
            xy1_prev[1] = xy1[1];
        }
        if (xy1[2] != xy1_prev[2])
        {
            sendtouch((xy1[2]), 0);
            xy1_prev[2] = xy1[2];
        }
    }
    
    if (XY2_DATAREADY_Read())
    {
        
        I2C_MasterReadBuf(xy2_address, xy2, 3, I2C_MODE_COMPLETE_XFER);
        timeout = 1000;
        while ((0u == (I2C_MasterStatus() & I2C_MSTAT_RD_CMPLT)) && (timeout > 0))
        {
            timeout--;
        }
           
        I2C_MasterClearStatus();
        if (xy2[0] != xy2_prev[0])
        {
            sendXY((xy2[0]), 2);
            xy2_prev[0] = xy2[0];
        }
        if (xy2[1] != xy2_prev[1])
        {
            sendXY((xy2[1]), 3);
            xy2_prev[1] = xy2[1];
        }
        if (xy2[2] != xy2_prev[2])
        {
            sendtouch((xy2[2]), 1);
            xy2_prev[2] = xy2[2];
        }
    }
} 
        
void sendButtons()
{
    if (buttons != prevButtons)
    {
        
        teensy_send[0] = buttonoffset;
        teensy_send[1] = 0;
        teensy_send[2] = (buttons & 127);
        sendI2CMidiData();      
        prevButtons = buttons;
    }    
}

/* [] END OF FILE */
