
#include <Arduino.h>
#include <SPI.h>

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
    const char message[] = "nrf_bench";

    radio.setChannel(108);
    radio.setPALevel(RF24_PA_MIN);
    radio.setDataRate(RF24_1MBPS);
    radio.openWritingPipe(address);
    radio.stopListening();

    while (1)
    {
        bool ok = radio.write(message, sizeof(message));
        Serial.println(ok ? F("TX ok") : F("TX failed"));
        delay(500);
    }

    return 0;
}
