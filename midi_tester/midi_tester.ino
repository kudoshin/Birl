/* USB MIDI AnalogControlChange Example

   You must select MIDI from the "Tools > USB Type" menu
   http://www.pjrc.com/teensy/td_midi.html

   This example code is in the public domain.
*/

#include <Bounce.h>
#include <Wire.h>
#define numValues 100

// the MIDI channel number to send messages
const int channel = 1;
int inc = 0;
int previousValues[numValues];
int currentValues[numValues];


// the MIDI continuous controller for each analog input
const int baseCC = 30;

int message[4] = {0,0,0,0};


elapsedMillis msec = 0;


void setup()
{  
  // zero out the array of stored values
  for (int i = 0; i < numValues; i++)
  {
    currentValues[i] = 0;
    previousValues[i] = -1;
  }
  Wire.begin(0);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event
}



// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
  
  // retrieve the message and store it in and array called message[], where the structure is {CC#, High Byte, Low Byte}
  int which = 0;
  while(1 < Wire.available()) // loop through all but the last
  {
    int x = Wire.read();    // receive byte as an integer
    message[which] = x;
    which++;
  }
  
  // now add the new information in message[] to the stored array of current values
  currentValues[message[0]] = (message[1] << 7) + (message[2] & 127);
  
  if (currentValues[message[0]] != previousValues[message[0]])
  {
     usbMIDI.sendControlChange((baseCC + (message[0] * 2)), message[1], channel); // print the integer
     usbMIDI.sendControlChange((baseCC + (message[0] * 2) + 1), message[2], channel); // print the integer
     previousValues[message[0]] = currentValues[message[0]];
  }
 

}



void loop() {
  // MIDI Controllers should discard incoming MIDI messages.
  // http://forum.pjrc.com/threads/24179-Teensy-3-Ableton-Analog-CC-causes-midi-crash
  while (usbMIDI.read()) {
    // ignore incoming messages
  }
}

