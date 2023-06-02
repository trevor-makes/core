#include <core.h>

#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO)
CORE_PORT(B)
using LEDPin = PortB::Bit<5>;

#elif ARDUINO_ARCH_RENESAS
CORE_PORT(1)
using LEDPin = Port1::Bit<11>;
#endif

void setup() {
  // Configure pin as output
  // DDRB |= (1 << 5); (AVR)
  // R_PFS->PORT[1].PIN[11].PmnPFS = bit(2); (Renesas)
  LEDPin::config_output();
}

// Blink LED at 1/2 Hz
void loop() {
  // Set pin high for 1 second
  // PORTB |= (1 << 5); (AVR)
  // R_PORT1->POSR = bit(11); (Renesas)
  LEDPin::set();
  delay(1000);

  // Set pin low for 1 second
  // PORTB &= ~bit(5); (AVR)
  // R_PORT1->PORR = bit(11); (Renesas)
  LEDPin::clear();
  delay(1000);
}
