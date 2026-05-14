#include <Arduino.h>
#include <avr/interrupt.h>

#include <SPI.h>
#include "RF24.h"

#define CE_PIN 7
#define CSN_PIN 8
// instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);

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
const uint32_t PAUSE_THRESHOLD = 2000; // ms, adjust as needed

void handleInterrupt()
{
    cli();
    uint32_t now = timer_ms;

    elapsed = now - last_peak_time; // Time between two toggles
    // Reset ignore logic if pause too lgtn
    if (elapsed >= PAUSE_THRESHOLD)
    {
        ignore_toggle = 0;
    }

    // Count every two toggles
    if (ignore_toggle == 1)
    {
        pin_high = true;
        interrupt_count++;
    }
    last_peak_time = now;
    ignore_toggle ^= 1; // Toggle between 0 and 1
    sei();
}

// Timer1 interrupt every 1ms
ISR(TIMER1_COMPA_vect)
{
    timer_ms++;
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

    if (!radio.begin())
    {
        Serial.println(F("radio hardware is not responding!!"));
        while (1)
        {
        } // hold in infinite loop
    }
    radio.setAutoAck(true);
    radio.enableDynamicPayloads();

    uint8_t address[] = "abcde";
    radio.setChannel(108);
    radio.openReadingPipe(0, address);
    radio.setPALevel(RF24_PA_MIN);
    radio.setDataRate(RF24_1MBPS);
    radio.startListening();

    Serial.println(F("RF24 receiver initialized"));

    uint8_t radio_buf[32] = {0};
    uint32_t previous_revolution_time = timer_ms;

    while (1)
    {
        // Check for incoming radio data
        if (radio.available())
        {
            uint8_t len = radio.getPayloadSize();
            Serial.print(F("Received data length: "));
            Serial.println(len);
            if (len > 32)
                len = 32;
            radio.read(radio_buf, len);
            Serial.print(F("RX: "));
            for (uint8_t i = 0; i < len; i++)
            {
                if (radio_buf[i] < 16)
                    Serial.print(F("0"));
                Serial.print(radio_buf[i], HEX);
                if (i < len - 1)
                    Serial.print(F(" "));
            }
            Serial.println();
        }
        else
        {
            // Serial.print(F("No radio data. "));
        }

        if (pin_high)
        {
            uint16_t revolution_time = 0;
            if (timer_ms - previous_revolution_time <= PAUSE_THRESHOLD)
            {
                revolution_time = timer_ms - previous_revolution_time;
            }
            previous_revolution_time = timer_ms;
            Serial.print("C: ");
            Serial.print(interrupt_count);
            Serial.print(", T: ");
            Serial.println(revolution_time);

            pin_high = false;
        }
    }

    return 0;
}
