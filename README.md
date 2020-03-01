# Incident Counter

## Overview

An automatic incident counter running on an Arduino Pro Micro. The counter records the number of days since the last incident occurred. There is no network connectivity, calendaring, or adjustment for daylight savings - a day is exactly 24 hours long.

<img alt="image of the counter" src="https://raw.githubusercontent.com/dsarratt/incidentcounter/master/images/01.jpg" width="300px"> <img alt="image of the counter" src="https://raw.githubusercontent.com/dsarratt/incidentcounter/master/images/02.jpg" width="300px">

## Physical layout

The incident counter has two inputs:

1. A toggle switch, used to record an incident.
2. A pushbutton, used to adjust screen brightness and manually increment the day count.

## Operation

1. When the device is first turned on it will start in "idle mode", which displays the number of days since the last incident. This day counter will automatically increment every 24 hours.
1. Toggle the incident switch to enable "incident mode" - during incident mode the display will flash "HELP". Disabling incident mode will return the device to idle status and reset the day count to zero.
1. To adjust screen brightness, press and hold the pushbutton for at least 750ms. The screen will display "br" followed by the current screen brightness. Tap the pushbutton to cycle through brightness levels (0-15).
1. To manually increment the day count, press and hold the pushbutton while in brightness mode. The display will show a colon (:) followed by the number of days since the last incident. Tap the pushbutton to increase the day count.
1. The device will save any changes and revert to "idle mode" if the buttons are left untouched for seven seconds.
1. To reset the day count to zero, toggle the incident switch on and off while the device is in "idle mode".

## Bill of Materials

The device is powered by the [SparkFun Pro Micro Arduino](https://www.sparkfun.com/products/12640), I used the 5V/16Mhz version but it should work with the 3V variety. The LED display is the [1.2" 7-segment LED backpack from AdaFruit](https://learn.adafruit.com/adafruit-led-backpack/1-2-inch-7-segment-backpack), this requires a 5V input. Power is provided by a generic 5V microUSB charger plugging directly into the Arduino, required current is low (e.g. less than 200mA).

I used a small piece of stripboard to connect the wires and mount the Arduino, with a few headers to plug in the buttons and LED backpack. The Arduino pins used are:

* VCC, regulated power supply to the 7-segment backpack
* GND, to the 7-segment backpack and to the buttons
* A3 and A2, I/O pins to the buttons
* SDA and SCL (I/O pins 2 and 3), to the 7-segment backpack

Button inputs are pulled HIGH by the Arduino, pressing the button or enabling the toggle will drive the inputs LOW. Buttons are debounced in software.

## Code layout

The device uses a state machine to move between the different modes:

![State Machine Diagram](/images/State%20Machine.png)

Code is in C++ and was compiled using the Arduino IDE 1.8.9. It uses the LED backpack libraries provided by AdaFruit, details for setting this up can be found on [AdaFruit's website](https://learn.adafruit.com/adafruit-led-backpack/1-2-inch-7-segment-backpack-arduino-wiring-and-setup).
