/***************************************************
Incident clock

Donald Sarratt
2019-08-23

Note the implicit Arduino code layout:
setup() is called once when the device initialises
loop() is the main program loop
****************************************************/

#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

// Pointer to the 7-seg object
Adafruit_7segment matrix = Adafruit_7segment();

// This doesn't seem to be in the library
enum seg_bars {
    BAR_TOP = 0x1,
    BAR_TOP_RIGHT = 0x2,
    BAR_BOTTOM_RIGHT = 0x4,
    BAR_BOTTOM = 0x8,
    BAR_BOTTOM_LEFT = 0x10,
    BAR_TOP_LEFT = 0x20,
    BAR_MIDDLE = 0x40,
    BAR_DOT = 0x80,
};

// Current brightness of LEDs (0-15)
char brightness = 0;

// Track days since last incident
unsigned int days = 0;

// Store the millis() when our last day started
unsigned long prev_millis = 0;

// Alterable for testing purposes, length of a day in milliseconds
static const unsigned long DAYLENGTH = 1000UL*60*60*24;
//static const unsigned long DAYLENGTH = 1000UL*60;

// EEPROM address for storing day count (two-byte int)
static const int EEP_DAY = 0;

// EEPROM address for storing brightness (one-byte int)
static const int EEP_BRG = 5;

// Which pin is the "incident" toggle?
static const int PIN_INC = A3;
// Which pin is the pushbutton?
static const int PIN_BTN = A2;

// Possible button press durations
enum button_length {SHORTPRESS, LONGPRESS};

// How long is a long press (ms)?
static const int LONG_PRESS_DURATION = 750;

// How long do we sit in a config menu before leaving?
static const int MENU_TIMEOUT = 7000;

void setup() {
#ifndef __AVR_ATtiny85__
    //Serial.begin(9600);
    //Serial.println("Hello");
    //Serial.println(DAYLENGTH);
#endif
    
    // Read current days, brightness out of EEPROM
    days = EEPROM.get(EEP_DAY, days);
    brightness = EEPROM.get(EEP_BRG, brightness);
    
    // 0x70 is the I2C address of our display
    matrix.begin(0x70);
    
    // Initialise display
    matrix.setBrightness(brightness);
    matrix.print(days, DEC);
    matrix.writeDisplay();
    
    // Set input pins for incident toggle and pushbutton
    pinMode(PIN_INC, INPUT_PULLUP);
    pinMode(PIN_BTN, INPUT_PULLUP);
}

void check_day_count() {
    /*
    Compare current time to previous, if a day has elapsed
    then increment our counter.
    */
    if ((unsigned long)(millis() - prev_millis) >= DAYLENGTH) {
        // Increment daycount, reset prev_millis
        // Note the implicit unsigned rollover on prev_millis
        days += 1;
        prev_millis += DAYLENGTH;
        
        // Save daycount to EEPROM
        EEPROM.put(EEP_DAY, days);
        
        // Write new daycount to display
        matrix.print(days, DEC);
        matrix.writeDisplay();
    }
}

void brightness_mode() {
    /* Brightness setting mode */
    // Set display, e.g. 'br 01'
    matrix.writeDigitRaw(0, BAR_TOP_LEFT | BAR_BOTTOM_LEFT | BAR_BOTTOM
                            | BAR_MIDDLE | BAR_BOTTOM_RIGHT);
    matrix.writeDigitRaw(1, BAR_BOTTOM_LEFT | BAR_MIDDLE);
    matrix.writeDigitRaw(2, 2);     // Colon
    matrix.writeDigitNum(3, brightness / 10);
    matrix.writeDigitNum(4, brightness % 10);
    // matrix.blinkRate(HT16K33_BLINK_1HZ);
    matrix.writeDisplay();
    
    // Wait for button to be released
    while (digitalRead(PIN_BTN) == LOW) {
        delay(50);
    }
    
    // Wait for a button press
    int idle = 0;
    while (idle < MENU_TIMEOUT) {
        if (digitalRead(PIN_BTN) == HIGH) {
            delay(10);
            idle += 10;
            continue;
        }
        if (get_presslength() == SHORTPRESS) {
            idle = 0;
            brightness += 1;
            if (brightness > 15) {
                brightness = 0;
            }
            matrix.setBrightness(brightness);
            matrix.writeDigitNum(3, brightness / 10);
            matrix.writeDigitNum(4, brightness % 10);
            matrix.writeDisplay();
        } else {
            // Long press, move to day incrementer
            day_mode();
            break;
        }
    }
    
    // Store the new brightness before returning
    EEPROM.update(EEP_BRG, brightness);
    // Reset display
    matrix.print(days, DEC);
    // matrix.blinkRate(HT16K33_BLINK_OFF);
    matrix.writeDisplay();
    
    // Wait for the button to be released before properly exiting
    while (digitalRead(PIN_BTN) == LOW) {
        delay(50);
    }
}

void day_mode() {
    /* Day incrementing mode */
    // Set display
    matrix.print(days, DEC);
    matrix.writeDigitRaw(2, 0x4|0x8);   // Left colon
    matrix.writeDisplay();
    
    // Wait for button to be released
    while (digitalRead(PIN_BTN) == LOW) {
        delay(50);
    }
    
    // Wait for a button press
    int idle = 0;
    while (idle < MENU_TIMEOUT) {
        if (digitalRead(PIN_BTN) == HIGH) {
            delay(10);
            idle += 10;
            continue;
        }
        if (get_presslength() == SHORTPRESS) {
            idle = 0;
            days += 1;
            matrix.print(days, DEC);
            matrix.writeDigitRaw(2, 0x4|0x8);   // Left colon
            matrix.writeDisplay();
        } else {
            // Long press, exit the setup
            break;
        }
    }
    
    // Store the new daycount
    EEPROM.put(EEP_DAY, days);
}

enum button_length get_presslength() {
    /* Wait for the pushbutton to be released, then
    report the push duration.
    */
    // Wait 100ms to debounce
    delay(100);
    int total_wait = 100;
    
    // Wait for switch to turn off, or longpress to activate
    while (digitalRead(PIN_BTN) == LOW && total_wait < LONG_PRESS_DURATION) {
        delay(50);
        total_wait += 50;
    }
    
    // Return the duration of the press
    if (total_wait >= LONG_PRESS_DURATION)
        return LONGPRESS;
    else
        return SHORTPRESS;
}

void incident_loop() {
    /* Idle loop while the incident pin is high */
    // Setup, zero the display and make it blink
    // matrix.writeDigitRaw(0, BAR_MIDDLE);
    // matrix.writeDigitRaw(1, BAR_MIDDLE);
    // matrix.writeDigitRaw(3, BAR_MIDDLE);
    // matrix.writeDigitRaw(4, BAR_MIDDLE);
    // Spell 'HELP' on the display
    matrix.writeDigitRaw(0, 0xFF & ~BAR_TOP & ~BAR_BOTTOM);
    matrix.writeDigitRaw(1, 0xFF & ~BAR_TOP_RIGHT & ~BAR_BOTTOM_RIGHT);
    matrix.writeDigitRaw(3, BAR_TOP_LEFT | BAR_BOTTOM_LEFT | BAR_BOTTOM);
    matrix.writeDigitRaw(4, 0xFF & ~BAR_BOTTOM & ~BAR_BOTTOM_RIGHT);
    // matrix.blinkRate(HT16K33_BLINK_1HZ);
    matrix.writeDisplay();
    
    // Wait for the incident to be over
    while (digitalRead(PIN_INC) == LOW) {
        Wire.beginTransmission(0x70);
        Wire.write(HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON); 
        Wire.endTransmission();
        delay(800);
        Wire.beginTransmission(0x70);
        Wire.write(HT16K33_BLINK_CMD | 0); 
        Wire.endTransmission();
        delay(200);
    }
    
    // Reset day count, set display back to total days
    days = 0;
    EEPROM.put(EEP_DAY, days);
    prev_millis = millis();
    matrix.print(days, DEC);
    matrix.blinkRate(HT16K33_BLINK_OFF);
    matrix.writeDisplay();
}

void loop() {
    // If incident pin is enabled, go to the incident loop
    if (digitalRead(PIN_INC) == LOW) {
        incident_loop();
    }
    
    // If we get a long button press, move to brightness-setting mode
    if (digitalRead(PIN_BTN) == LOW) {
        if (get_presslength() == LONGPRESS) {
            brightness_mode();
        } else {
            // Short press, ignore it
        }
    }
    
    // Check timer count
    check_day_count();
    
    // Wait a bit before the next loop
    delay(10);
}
