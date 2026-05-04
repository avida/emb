
#include <Arduino.h>

#define INTERRUPT_PIN 2 // Use digital pin 2 for external interrupt (INT0 on most AVRs)

volatile bool pin_high = false;

void handleInterrupt() {
    if (digitalRead(INTERRUPT_PIN) == HIGH) {
        pin_high = true;
    }
}


int main(void)
{
    init();
    Serial.begin(57600);
    Serial.println("Hi");

    pinMode(INTERRUPT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handleInterrupt, RISING);

    while (1) {
        if (pin_high) {
            Serial.println("Interrupt: pin is HIGH");
            pin_high = false;
        }
    }

    return 0;
}
