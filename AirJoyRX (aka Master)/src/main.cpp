/*

AirJoy 'Master'
---------------

Copyright (c) 2017, Martin Böss (The Don, Don M)

This work is licensed under the terms of the GNU General Public License version 3.
You should have received a copy of the GNU Lesser General Public License along with this software; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA



This is the firmware for presenting 4 USB Game-Controllers to a
connected USB-Host, receiving data from AirJoy 'AirStick'(s) and
feeding the data to the connected host.

An Arduino Pro Mirco in combination with a nRF24L01+
board is used to receive the data from currently 4
independant transmitters. The data then gets transformed
into USB-HID reports which in turn get send to a connected
host.



HARDWARE - SETUP
-----------------

Radio nRF24L01+ -> ARDUINO PRO MINI
CE    -> 9                          * can be configured
CSN   -> 10 (Hardware SPI SS)       * can be configured
MOSI  -> 11 (Hardware SPI MOSI)
MISO  -> 12 (Hardware SPI MISO)
SCK   -> 13 (Hardware SPI SCK)
IRQ   -> No connection
VCC   -> No more than 3.6 volts !!!
GND   -> GND

i used a 5v/16MHz Arduino connected to USB, powering the nRF24L01+
board through a lf33cv line-regulator.

SOFTWARE - Configuration
-----------------

nothing yet



 TODO:

- set report to x:0 y:0 b:0 on connection lost
  how do i know? maybe n secs no recv !

- auto-mask feature : random mask, get from joy at start 10x

- no joy.begin b4 radio ok

- alt-mode: one ore more modifier-switch(es) (like shift on keyboards) maps
  the input from joy to another set of buttons, e.g for controlling emlators, going
  into the menu in retroarch, etc.. need to add buttons to TX and RX, then


*/


#include <SPI.h>
#include <NRFLite.h>
#include <stdio.h>
#include <Joystick.h>



#define APP_VERSION     "AirJoy Master v0.3"

#define RELEASE         // def away all serial chitchat in the main-loop


// this is for me :)
#ifdef ARDUINO_AVR_MINI
    #error " C'mon - this is for the ProMicro !!!"
#endif



static FILE uartout = {0} ;
static int uart_putchar (char c, FILE *stream)
{
    Serial.write(c) ;
    return 0 ;
}

const static uint8_t RADIO_ID = 0;       // our radio_id   joy1=1 joy2=2 ...
const static uint8_t PIN_RADIO_CE = 9;
const static uint8_t PIN_RADIO_CSN = 10;


#define RF_MASK 0xCAFE

struct RadioPacket // this is what we get from the sender
{
    uint8_t   FromRadioId;
    byte      xAxis;
    byte      yAxis;
    uint8_t   buttons;
    uint8_t   FailedTx;
    uint16_t  RF_Mask;


};

NRFLite _radio;
RadioPacket _radioData;
#define RXLED 17   // signalling rf-error


#define JOYSTICK_COUNT 4

Joystick_ Joystick[JOYSTICK_COUNT] = {
  Joystick_ (0x03,JOYSTICK_TYPE_GAMEPAD,
    2, 0,                  // Button Count, Hat Switch Count
    true, true, false,     // X and Y, but no Z Axis
    false, false, false,   // No Rx, Ry, or Rz
    false, false,          // No rudder or throttle
    false, false, false),  // No accelerator, brake, or steering

  Joystick_ (0x04,JOYSTICK_TYPE_GAMEPAD,
    2, 0,                  // Button Count, Hat Switch Count
    true, true, false,     // X and Y, but no Z Axis
    false, false, false,   // No Rx, Ry, or Rz
    false, false,          // No rudder or throttle
    false, false, false),  // No accelerator, brake, or steering

  Joystick_ (0x05,JOYSTICK_TYPE_GAMEPAD,
    2, 0,                  // Button Count, Hat Switch Count
    true, true, false,     // X and Y, but no Z Axis
    false, false, false,   // No Rx, Ry, or Rz
    false, false,          // No rudder or throttle
    false, false, false),  // No accelerator, brake, or steering

  Joystick_ (0x06,JOYSTICK_TYPE_GAMEPAD,
    2, 0,                  // Button Count, Hat Switch Count
    true, true, false,     // X and Y, but no Z Axis
    false, false, false,   // No Rx, Ry, or Rz
    false, false,          // No rudder or throttle
    false, false, false),  // No accelerator, brake, or steering

  };



void setup() {


    for (int i = 0; i < JOYSTICK_COUNT; i++) {
      Joystick[i].setXAxisRange(0, 255);
      Joystick[i].setYAxisRange(0, 255);
      Joystick[i].setXAxis(128);
      Joystick[i].setYAxis(128);
      Joystick[i].setButton(0,0);
      Joystick[i].setButton(1,0);
      Joystick[i].setButton(2,0);
      Joystick[i].setButton(3,0);

    }
    for (int i = 0; i < JOYSTICK_COUNT; i++)
      Joystick[i].begin();



    Serial.begin(57600);
    fdev_setup_stream (&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);    // for a nice printf
    stdout = &uartout ;


    delay(2000);  // wait for uart-enumeration
    Serial.println(APP_VERSION);




    if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN )) //, NRFLite::BITRATE250KBPS ))
    {
        Serial.println("Cannot communicate with radio");

        SPI.end();
        pinMode(RXLED, OUTPUT);
        while (1) { // forever :(
          digitalWrite(RXLED, LOW);
          TXLED0;
          delay(333);
          digitalWrite(RXLED, HIGH);
          TXLED1;
          delay(333);
        }
    }

    Serial.println("Communication with radio established");
    //_radio.printDetails();
    printf("RF-Mask: %X\n", RF_MASK);




    Serial.print(JOYSTICK_COUNT,DEC);
    Serial.println(" Air-Joysticks initialized");




}




#ifndef RELEASE
unsigned long rx_last = 0;
uint16_t timer = 0;
#endif

void loop() {

  while (_radio.hasData())    {

      _radio.readData(&_radioData);

      #ifndef RELEASE
      printf("\nRX  id: %3d   dT: %5ld   err: %3d   ",
          _radioData.FromRadioId,
          millis() - rx_last,
          _radioData.FailedTx);
          printf("x: %3d   y: %3d   b: %3d  rf_msk: %X",
          _radioData.xAxis,     //0 - 128 - 255
          _radioData.yAxis,
          _radioData.buttons,
          _radioData.RF_Mask
      );
      rx_last = millis();
      #endif

      _radioData.FromRadioId--;   // to array index

      if( _radioData.RF_Mask == RF_MASK ){      // skip accidentally received data


          Joystick[_radioData.FromRadioId].setXAxis(_radioData.xAxis);
          Joystick[_radioData.FromRadioId].setYAxis(_radioData.yAxis);
          Joystick[_radioData.FromRadioId].setButton(0, _radioData.buttons & 0b0001);
          Joystick[_radioData.FromRadioId].setButton(1, _radioData.buttons & 0b0010);
          Joystick[_radioData.FromRadioId].setButton(2, _radioData.buttons & 0b0100);
          Joystick[_radioData.FromRadioId].setButton(3, _radioData.buttons & 0b1000);





      #ifndef RELEASE
        Serial.print(" - accepted ");
      #endif
      Serial.print(""); // the masters tx led is flashing when printing - nice indicator!
      }
      #ifndef RELEASE
      else
      {
        Serial.print(" - rejected ");
      }
      #endif

      // report gets autosend

  }


  #ifndef RELEASE
  if(timer++ > 50000){
      Serial.print(".");
      timer = 0;
  }
  #endif


 }
