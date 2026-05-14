#include <SSD1306.h>
#include <Wire.h>

#include <Arduino.h>

int main(void)
{
    init();

    Serial.begin(57600);
    Serial.println("SSD1306 demo");
    SSD1306 display(SSD1306_HEIGHT_32);
    Wire.begin();


    display.init();
    display.clear();

    display.print('!');
    display.print(F("\"#$%&'()*+,-./"));
    display.setCaret(10, 10);
    display.setScale(2);
    display.print(F("0123456789"));
    display.setCaret(40, 28);
    display.setScale(1);
    display.print(F(":;<=>?@"));
    display.setCaret(0, 40);
    display.print(F("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
    display.print(F("[\\]^_`"));
    display.print(F("abcdefghijklmnopqrstuvwxyz"));
    display.print(F("{|}~"));

    display.update();
}