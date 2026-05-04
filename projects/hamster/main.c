

#include <Arduino.h>
#include <avr/interrupt.h>

#define INTERRUPT_PIN 2 // Digital pin 2 (INT0)

volatile bool pin_high = false;
volatile uint32_t interrupt_count = 0;
volatile uint32_t timer_ms = 0;
volatile uint32_t last_peak_time = 0;
volatile uint32_t elapsed = 0;
volatile uint8_t ignore_toggle = 0;
// If wheel takes to long to make a revolution we consider it to be stopped so
// we can calculate momentary speed correctly when it starts moving again. This
// is needed because we ignore every second trigger to get accurate count of
// revolutions, so if wheel stops for a long time we need to reset that logic to
// avoid missing first trigger when it starts moving again.
const uint32_t PAUSE_THRESHOLD = 2500; // ms, adjust as needed

void handleInterrupt()
{
    if (digitalRead(INTERRUPT_PIN) == HIGH)
    {
        uint32_t now = timer_ms;
        elapsed = now - last_peak_time;
        if (elapsed > PAUSE_THRESHOLD)
        {
            elapsed = 0;
        }
        last_peak_time = now;
        if (ignore_toggle == 0)
        {
            pin_high = true;
            interrupt_count++;
        }
        // When magnet passing through reed switch, it causes two triggers (one for left and one for right plate).
        // We want to ignore every second trigger to get accurate count of revolutions.
        ignore_toggle ^= 1; // Toggle between 0 and 1
    }
}

// Timer1 interrupt every 1ms
ISR(TIMER1_COMPA_vect)
{
    timer_ms++;
    // Reset ignore logic if pause too long so we can ignore first trigger when
    // wheel starts moving again after a long stop.
    if ((timer_ms - last_peak_time) > PAUSE_THRESHOLD)
    {
        ignore_toggle = 0;
    }
}

void setup_timer1_1ms()
{
    cli();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    OCR1A = 249; // 16MHz/64/250 = 1kHz (1ms)
    TCCR1B |= (1 << WGM12);
    TCCR1B |= (1 << CS11) | (1 << CS10); // Prescaler 64
    TIMSK1 |= (1 << OCIE1A);
    sei();
}

int main(void)
{
    init();
    Serial.begin(57600);
    Serial.println("Hi");

    setup_timer1_1ms();

    pinMode(INTERRUPT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handleInterrupt, RISING);

    uint32_t last_print_count = 0;
    while (1)
    {
        if (pin_high)
        {
            Serial.print(interrupt_count);
            Serial.print(", ");
            Serial.println(elapsed);
            pin_high = false;
            last_print_count = interrupt_count;
        }
        delay(10);
    }

    return 0;
}
