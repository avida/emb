#include <Arduino.h>

#define SERVO_PIN 9
int main(void)
{
    init();
    Serial.begin(57600);
    Serial.println("Hi");

    return 0;
}
