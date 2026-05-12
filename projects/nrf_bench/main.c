
#include <Arduino.h>
#include <SPI.h>

#include <stdio.h>

#include "RF24.h"

#define CE_PIN 7
#define CSN_PIN 8

RF24 radio(CE_PIN, CSN_PIN);

int main(void)
{
    init();
    Serial.begin(57600);

    if (!radio.begin())
    {
        Serial.println(F("radio hardware is not responding"));
        while (1)
        {
        }
    }

    const uint8_t address[] = "abcde";
    uint32_t counter = 0;
    char message[32] = {0};

    radio.setChannel(108);
    radio.setPALevel(RF24_PA_MIN);
    radio.setDataRate(RF24_1MBPS);
    radio.openWritingPipe(address);
    radio.stopListening();

    while (1)
    {
        int message_len = snprintf(message, sizeof(message), "nrf%lu", static_cast<unsigned long>(counter));
        if (message_len < 0)
        {
            message_len = 0;
            message[0] = '\0';
        }

        bool ok = radio.write(message, static_cast<uint8_t>(message_len + 1));
        Serial.println(ok ? F("TX ok") : F("TX failed"));
        counter++;
        delay(1000);
    }

    return 0;
}
