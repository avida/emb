

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

uint8_t encoded_details[43] = {0};

void dumpRegData()
{
    for (uint8_t i = 0; i < 43; ++i)
    {
        Serial.print(encoded_details[i], HEX);
        if (i < 42)
            Serial.print(F(" "));
    }
}

int main(void)
{
    init();
    Serial.begin(57600);
    Serial.println("Hi");

    // setup_timer1_1ms();

    // pinMode(INTERRUPT_PIN, INPUT);
    // attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handleInterrupt, RISING);

    if (!radio.begin())
    {
        Serial.println(F("radio hardware is not responding!!"));
        while (1)
        {
        } // hold in infinite loop
    }
    radio.setAutoAck(true); // Don't acknowledge arbitrary signals
    radio.enableDynamicPayloads(); // Allow variable payload sizes

    // Configure RF24 as receiver
    uint8_t address[] = "abcde"; // Simple single-byte address for testing
    radio.setChannel(108); // Set to a less crowded channel (2.476 GHz)
    radio.openReadingPipe(0, address);
    radio.setPALevel(RF24_PA_MIN);
    radio.setDataRate(RF24_1MBPS);
    radio.startListening();

    Serial.println(F("RF24 receiver initialized"));

    uint32_t last_print_count = 0;
    uint8_t radio_buf[32] = {0};

    while (1)
    {
        // Check for incoming radio data
        if (radio.available())
        {
            uint8_t len = radio.getPayloadSize();
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
        } else {
            Serial.print(F("No radio data. "));
        }

        if (pin_high)
        {
            Serial.print(interrupt_count);
            Serial.print(", ");
            Serial.println(elapsed);
            pin_high = false;
            last_print_count = interrupt_count;
        }
        delay(1000);
    }

    return 0;
}
