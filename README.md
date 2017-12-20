# AirJoy

Wirelessly connect old-school Joysticks as USB-HID Gamepads !

## Preface:

I recently got into retro-gaming (again). The fantastic works of the RetroArch project and many others got me seriously hooked up, so I got me some DS3-Controllers to in turn hook them up to my Pi/Pc for playing those beloved games of my (long-gone :whine:) youth. 

But playing titles like Galaga Deluxe (A500, the BEST! version) or Giana-Sisters (C64) with those controllers wasn't the real thing IMHO. On the other hand, hooking up my trusty joy and being bound by lengthy cables was not state of the art, I thought. 

So to overcome the shortcomings of lengthy cables, I summoned the powers of AtMega and mixed together some ingredients I had laying around to marry-up old-school-circuitry with next-gen-electronics!

Please note that this is the first time for me publishing a project of mine to an open community. Bear with me, if things don't work on your system, code is bad, buggy or burning-your-devices and the like. And, in anticipation of problems to arise,  I make this inevitable 



## Disclaimer:

> THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

		

## Description:


The projects main purpose is to present 4 USB Game-Controllers (2 axis, 2 buttons) to a connected USB-Host, receiving data wirelessly from independent transmitters pysically connected to digital 'old-school' 'atari- or c64-style' Joysticks and feeding the data as usb-reports to the connected host.



## Thanks:


In direct conjunction with this project, I would like to thank:

	Dave Parson - for the easy to use NRFLite library 
	Matthew Heironimus - for the hassle-free ArduinoJoystickLibrary
	Rocket Scream Electronics - for the useful low-power library
	
	
	
Furthermore, I'd like to thank:

	retroarch/libretro 
	retropie
	emulationstation
	platformIO
	
	
	
	
Nuff' said - let's get to the nitty-gritty...


	

## Hardware:


This project consists of a receiving part and one ore more sending parts, referred to as 'Master' and 'AirStick'(s).

**Master:**

	Arduino Pro Micro
	2x nrf24l01+ board  (3.6v !!!)
	some sort of voltage-regulation 5v -> 3.6v
	
**AirStick:**

	Arduino Pro Mini
	Digital 'old-school' Joystick with UP/DOWN/LEFT/RIGHT stick and 2 pushbuttons
	nrf24l01+ board (3.3v !!!)
	some sort of  battery
	some sort of voltage-regulation 5v -> 3.6v

	
**Notes on Hardware:**

Since neither of the used Arduinos provide 3.3 volts, I used 

* For the Master:

a 5v/16MHz Arduino connected to USB, powering the nRF24L01+ board through a lf33cv line-regulator 

* For the AirStick(s):

a 5v/16MHz Arduino at 3.3 volts from a power-source and from a LI-ION battery, powering the nRF24L01+ through the arduinos vcc

But YMMV !


**Notes on battery-life:**

Since the AirStick is meant to be run on batteries, I tried to lower the power consumption of the hardware by means of software by incorporating the Rocket Scream Electronics Low-Power library. The utilisation of powersaving features is currenty far from completion, but power consumption is lowered to a level usable in real-life scenarios.

When no updates from the joystick are registered, the firmware enters, after a certain grace-period of currenty * ~1 second, an idle state, powering off some components of the mcu. if this state gets entered repeatately 4 times, the device is send into a deeper powersaving state. pinchange-irqs will then wakeup the mcu, resuming operation. 

The power consumption in normal operation is roughly 10mA, and in powersaving, the device draws around 1mA. This way, the device may be powered from a 16500 900mA battery for theoretically 90 hours with constant updates, or idling for 37 days.



## Firmware:


The firmware of this project also consists of two parts: 

	* 'Master'   - the receiver and usb-hid interface
	* 'AirStick' - the transmitter and physical joystick interface
	
Precompiled versions of the firmware can be obtained at the [release page](https://github.com/glitterkitty/AirJoy/releases).
		
	

## Settings:


On the transmitting part, a Joystick ID is set on startup. When using more than one AirSticks, this ID needs to be set by pushing the stick UP for ID 1, DOWN for ID 2, LEFT for ID 3 or RIGHT for ID 4.

When using ID 4, a special test-mode for testing/debugging can be activated by additionally pressing Button 1 of the Joystick. In this mode, random data is send to the Master, simulating inputs for all available AirStick IDs. These will then show up as button-presses or axis-movements on the host system. Powering off the transmitting device will end this mode.



## Source-Code:


Because coding in the Arduino-IDE is major pain and setting up my favoured IDE NetBeans for Arduino is tricky, this project got coded in the Atom - Editor in conjunction with PlatformIO, which is very nice to work with (and looks cool, too)!

You might be able to rename the main.cpp files to main.ino and build them in the Arduino-IDE, provided the needed Libraries are properly installed, though I haven't tested this.




## Copyright and License:


Copyright (c) 2017, Martin Böss (The Don, Don M)
	
This software is licensed under the terms of the GNU GENERAL PUBLIC LICENSE Version 3.
	
	



## Libraries used:


Low-Power

	- Author: Rocket Scream Electronics
	- Version: 1.6 
	- URL: [Low-Power](https://github.com/rocketscream/Low-Power)

 NRFLite

	- Author: Dave Parson <dparson55@hotmail.com>
	- Version: 2.0.1
	- URL: [NRFLite](https://github.com/dparson55/NRFLite)

 Joystick

	- Author: Matthew Heironimus
	- Version: 2.0.4
	- URL: [ArduinoJoystickLibrary](https://github.com/MHeironimus/ArduinoJoystickLibrary)




Final Words:
-------------

Coded with pain and pleasure by The Don in 12/2017


