#include "uIO.hpp"

#include <Arduino.h>

void copy_portC_to_portD() {
    // Configure I/O port C as input
    using PortC = uIO::PortC;
    PortC::config_input(); // DDRC = 0x00; PORTC = 0x00; [OUT, OUT]

    // Configure I/O port D as output
    using PortD = uIO::PortD;
    PortD::config_output(); // DDRD = 0xFF; [LDI, OUT]

    // Read from port C and write value to port D
    PortD::write(PortC::read()); // PORTD = PINC; [IN, OUT]
}

// True for Arduino Uno; use different pin for other boards
using LEDPin = uIO::PinB5;

void setup() {
    // Configure pin as output
    LEDPin::config_output(); // DDRB |= (1 << 5); [SBI]
}

// Blink LED at 1/2 Hz
void loop() {
    // Set pin high for 1 second
    LEDPin::set(); // PORTB |= (1 << 5); [SBI]
    delay(1000);

    // Set pin low for 1 second
    LEDPin::clear(); // PORTB &= ~(1 << 5); [CBI]
    delay(1000);
}
