/*

Demonstrates simple TX operation with an ATtiny85.  ATtiny's have a Universal Serial Interface
peripheral (USI) that can be used for SPI communication, and NRFLite utilizes this capability.

Radio -> ATtiny85

CE    -> Physical Pin 2, Arduino 3         
CSN   -> Physical Pin 2, Arduino 3         
MOSI  -> Physical Pin 6, Arduino 1 (Hardware USI_DO)  
MISO  -> Physical Pin 5, Arduino 0 (Hardware USI_DI)  
SCK   -> Physical Pin 7, Arduino 2 (Hardware USI_SCK) 
IRQ   -> No connection
VCC   -> No more than 3.6 volts
GND   -> GND

*/

#include <NRFLite.h>

const static uint8_t RADIO_ID = 4;             // Our radio's id.
const static uint8_t DESTINATION_RADIO_ID = 0; // Id of the radio we will transmit to.
const static uint8_t PIN_RADIO_CE = 3;
const static uint8_t PIN_RADIO_CSN = 3;

struct RadioPacket // Any packet up to 32 bytes can be sent.
{
    uint8_t FromRadioId;
    uint32_t OnTimeMillis;
    uint32_t FailedTxCount;
};

NRFLite _radio;
RadioPacket _radioData;

void setup()
{
    if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN))
    {
        while (1); // Cannot communicate with radio.
    }

    _radioData.FromRadioId = RADIO_ID;
}

void loop()
{
    _radioData.OnTimeMillis = millis();

    if (!_radio.send(DESTINATION_RADIO_ID, &_radioData, sizeof(_radioData))) // Note how '&' must be placed in front of the variable name.
    {
        _radioData.FailedTxCount++;
    }

    delay(1000);
}
