/*

AirJoy 'AirStick'
---------------

Copyright (c) 2017, Martin Böss (The Don, Don M)

This work is licensed under the terms of the GNU General Public License version 3.
You should have received a copy of the GNU Lesser General Public License along with this software; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA



This is the firmware for transmitting inputs from a connected Joystick
to an AirJoy 'Master'.

The code  utilizes an Arduino Pro Mini (clone) and a nRF24L01+
board. A digital Joystick gets connected to D2-D7. The
input of the Joystick then gets transmitted to Master,
which reports the data via USB-HID to a connected host.



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

i have run a 5v/16MHz Arduino at 3.3 volts and from a LI-ION battery,
powering the nRF24L01+ through the arduinos vcc, but YMMV



Joystick -> ARDUINO PRO MINI
b1  ->  d2
b2  ->  d3
up  ->  d4
dn  ->  d5
lf  ->  d6
rt  ->  d7
all are switches that connect to a common ground if pressed!



SOFTWARE - Configuration
-----------------

on startup the Joystick ID can be set.
when the led is on, push stick

    UP   for id 1
    DN   for id 2
    LF   for id 3
    RT   for id 4

till the led goes off

this then sets the FromID in the report and
also the RadioID used to init the rf-board.
not pushing defaults to 1 after ~3secs

a test-mode gets enabled when pressing button 1 and pushing
stick RT. this sends random data as joy 1-4 for testing the RX/TX





TODO:   -disable RF-Board when powerdown
        -monitor vcc to prevent deep-discharge when on battery (using internal 1.1vref!)
        -reduce fcpu for powersaving ?  https://forum.arduino.cc/index.php?topic=271364.0
        -debounce better in soft- or hardware
        -auto-mask feature : random mask, send to rx 10x at start
        -alt-mode: one ore more modifier-switch(es) (like shift on keyboards) maps
         the input from joy to another set of axis/buttons, e.g for controlling emlators, going
         into the menu in retroarch, etc.. need to add buttons to tx and rx, then





*/

#include <Arduino.h>
#include <SPI.h>
#include <stdio.h>
#include <avr/power.h>
#include <LowPower.h>
#include <NRFLite.h>



#define APP_VERSION     "AirJoy AirStick v0.3"

#define RELEASE             // def away all serial chitchat in the main-loop
#define JOY1234TEST         // enables test_mode


// just for me :)
#ifdef ARDUINO_AVR_LEONARDO
    #error " !!! No, you don't wanna accidently upload this to your ProMirco, again !!! "
#endif


// i like printf
static FILE uartout = {0} ;
static int uart_putchar (char c, FILE *stream)
{
    Serial.write(c) ;
    return 0 ;
}


// settings for the rf-board
      static uint8_t SOURCE_RADIO_ID;          // our radio's id, gets set on startup
const static uint8_t DESTINATION_RADIO_ID = 0; // Id of the radio we will transmit to, aka the AirJoyRX
const static uint8_t PIN_RADIO_CE = 9;         // see above *
const static uint8_t PIN_RADIO_CSN = 10;       // see above *


// structure of the data to be sent
#define RF_MASK 0xCAFE   // to filter accidentally received data, rx needs the same!
struct RadioPacket {

    uint8_t   JoyID;
    uint8_t   xAxis;
    uint8_t   yAxis;
    uint8_t   buttons;              // only 2 are used by now
    uint8_t   FailedTx;
    uint16_t  RF_Mask;
};

NRFLite _radio;         // our radio
RadioPacket report;     // the data


// powersave settings
#define INIT_IDLE_CNT   4                // go to powerdown after n idle cycles a 4s
#define INIT_GRACE 400000                // dont go immediately idle after update from joy,  400000 = ~1s


// forwards, declarations
int test_mode = 0;                       // for powerdoselection at startup
volatile unsigned long idle_cnt = 0;     // for powerdown
void setupPinChangeIRQ();                // init the pinchange irqs



void setup(){


    // Todo
    #ifndef RELEASE
    if(F_CPU == 8000000) {
      clock_prescale_set(clock_div_2);

    }
    if(F_CPU == 4000000) {
      clock_prescale_set(clock_div_4);

    }
    if(F_CPU == 2000000) {
      clock_prescale_set(clock_div_8);

    }
    if(F_CPU == 1000000) {
      clock_prescale_set(clock_div_16);

    }
    #endif


    // init communication and printf
    Serial.begin(57600);
    fdev_setup_stream (&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
    stdout = &uartout ;
    Serial.println(APP_VERSION);

    // prepare input pins
    int i;
    for(i=2;i<8;i++){   // PD0 PD1 are TXD RXD !
      pinMode(i, INPUT_PULLUP);
      digitalWrite(i, HIGH);
    }


    // determine joysick- and radioid
    Serial.println("Set Joystick-ID. Move stick up/dn/lf/rt for id 1/2/3/4");
    SOURCE_RADIO_ID = 1;    // default
    pinMode(13,OUTPUT);
    digitalWrite(13,HIGH);

    uint8_t in = 0,  d4=0, d5=0, d6=0, d7=0, timeout = 60;        // quick hack. it's overkill - i know
    while(timeout--) {

        in = ~(PIND >> 4) & 0x0F;
        switch(in){
            case 1:     d4++;
                        break;
            case 2:     d5++;
                        break;
            case 4:     d6++;
                        break;
            case 8:     d7++;
                        break;
        }

        if(d4>0x10){
            SOURCE_RADIO_ID = 1;
            break;
        }
        if(d5>0x10){
            SOURCE_RADIO_ID = 2;
            break;
        }
        if(d6>0x10){
            SOURCE_RADIO_ID = 3;
            break;
        }
        if(d7>0x10){
            SOURCE_RADIO_ID = 4;
            test_mode = (~PIND>>2) & 0x01;
            break;
        }

        //printf("\n%5x      %5X   %5X   %5X   %5X   ", in, d4,d5,d6,d7);
        delay(50);

    }
    digitalWrite(13,LOW);


    // init radio wt defult speed
    if (!_radio.init(SOURCE_RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN )) //, NRFLite::BITRATE250KBPS))
    {
        // no radio ?
        Serial.println("Cannot communicate with radio");
        SPI.end();
        pinMode(13,OUTPUT);
        while (1) { // stay here forever and blink
          digitalWrite(13,HIGH);
          delay(333);
          digitalWrite(13,LOW);
          delay(333);
        }

    }
    Serial.println("Communication with radio established");



    // install irq handler
    setupPinChangeIRQ();


    // init joy datastructure
    report.JoyID = SOURCE_RADIO_ID;
    report.FailedTx = 0;
    report.xAxis = 128;
    report.yAxis = 128;
    report.buttons = 0;


    if (test_mode == 0)
        printf("Initialized as Joy %d", SOURCE_RADIO_ID);
    else
        printf("Initialized Test Mode");

}



// register pinchange irqs for d2 - d7
void setupPinChangeIRQ(){

    int i;
    for(i=2;i<8;i++){   // PD0/PD1 are TXD/RXD !

        *digitalPinToPCMSK(i) |= bit (digitalPinToPCMSKbit(i));  // enable pin
        PCIFR  |= bit (digitalPinToPCICRbit(i)); // clear any outstanding interrupt
        PCICR  |= bit (digitalPinToPCICRbit(i)); // enable interrupt for the group

    }
}


// handle pin change interrupt for D2 to D7 here
// if u 1 2 enable more pins, other irq-vectors need 2 b used
ISR (PCINT2_vect) {
  idle_cnt = 0;  // nothing much here, but the irq will wake up the mcu from powerdown

}



// some more globals for the main-loop
uint8_t pins_curr = 0;
uint8_t pins_last = 0;
uint8_t res;
uint32_t grace = INIT_GRACE;




void loop(){

    #ifdef JOY1234TEST

        // send random jostick movements on all 4 AirJoys
        while( test_mode == 1){

            // random data
            report.JoyID   = (random(0xFF) & 0b00000011) + 1;
            report.buttons = random(0xFF) & 0b00001111;   // on D2 D3
            report.yAxis = random(0xFF) ;
            report.xAxis = random(0xFF) ;
            report.RF_Mask = RF_MASK;


            // send
            res = _radio.send(DESTINATION_RADIO_ID, &report, sizeof(report)) ;
            if( res == 0 ) report.FailedTx++;


            // show me
            printf("\nTX-Test  joy-id: %d  x: %3d   y: %3d   b: %3d  e: %3d   s: %3d   ",
            report.JoyID,
            report.xAxis,
            report.yAxis,
            report.buttons, report.FailedTx,  res
        );


        // be nice
        delay(200);


        // da capo
        }

    #endif


    //
    // No Testing - so here comes The Real McCoy (tm)
    //


    // dont'go idle immediately
    grace--;


    // get pin state
    pins_curr = PIND;


    // get rid of d0/d1 = txd/rxd
    pins_curr = ~ (pins_curr>>2);


    // sth changed !
    if(pins_curr != pins_last){

        // remember
        pins_last = pins_curr;


        // forge a report
        report.buttons = pins_curr & 0b00000011;   // on D2 D3
        report.yAxis = pins_curr & 0b00000100 ? 0: pins_curr & 0b00001000 ? 255 : 128;
        report.xAxis = pins_curr & 0b00010000 ? 0: pins_curr & 0b00100000 ? 255 : 128;
        report.RF_Mask = RF_MASK;


        // send it
        res = _radio.send(DESTINATION_RADIO_ID, &report, sizeof(report)) ;
        if( res == 0 ) report.FailedTx++;


        // show me
        #ifndef RELEASE
            printf("\nTX  joy: %d   x: %3d   y: %3d   b: %3d  fails: %3d   grace: %ld",
                report.JoyID,
                report.xAxis,
                report.yAxis,
                report.buttons,
                report.FailedTx,
                grace
            );
            delay(50);
        #endif

    // don't go to sleep so soon
    grace = INIT_GRACE;

    }


    // well, long time no updade from the joy -> save power.
    // grace is uint, so keep it save by <= 10
    if(grace <= 10){

        // count idle cycles
        idle_cnt++;

        // many idle cycles -> save some more power
        if(idle_cnt > INIT_IDLE_CNT ){

            #ifndef RELEASE
            printf("\np-down");
            delay(50);
            #endif

            idle_cnt = 0;


            // powerdown the rf-board too
            // Todo


            // sleep - we will get awakened up from the pinchange-irq!
            LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);



        }else{


            #ifndef RELEASE
            printf("\nidle");
            delay(50);
            #endif

            // shut down some components
            LowPower.idle(
                SLEEP_4S,
                ADC_OFF,
                TIMER2_OFF,
                TIMER1_OFF,
                TIMER0_OFF,
                SPI_ON,
                USART0_OFF,
                TWI_OFF
            );

        }

        // prevent grace underflow in case of high INIT_GRACE setting
        grace = 10;

    }

}
