#include <Arduino.h>
#include <Wire.h>

#include <hd44780.h> // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h>

#define SERVO_PIN 9
static constexpr uint16_t kServoPeriodUs = 20000;
static constexpr uint16_t kServoMinUs = 500;
static constexpr uint16_t kServoCenterUs = 1500;
static constexpr uint16_t kServoMaxUs = 2500;
hd44780_I2Cexp lcd;

static void setup_timer1_servo_pwm()
{
    // Timer1 Fast PWM, TOP=ICR1 (mode 14), non-inverting on OC1A (D9).
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    TCCR1A |= _BV(COM1A1);
    TCCR1A |= _BV(WGM11);
    TCCR1B |= _BV(WGM13) | _BV(WGM12);
    TCCR1B |= _BV(CS11); // Prescaler 8        delay(1000);.

    // 16 MHz / 8 = 2 MHz => 0.5 us per tick.
    ICR1 = static_cast<uint16_t>((F_CPU / 8UL / 50UL) - 1UL);
    OCR1A = static_cast<uint16_t>(kServoCenterUs * 2UL);
}

static void set_servo_pulse_us(uint16_t pulse_us)
{
    if (pulse_us < kServoMinUs)
    {
        pulse_us = kServoMinUs;
    }
    if (pulse_us > kServoMaxUs)
    {
        pulse_us = kServoMaxUs;
    }
    OCR1A = static_cast<uint16_t>(pulse_us * 2UL);
}
#ifndef LCD_COLS
#define LCD_COLS 16
#endif

#ifndef LCD_ROWS
#define LCD_ROWS 2
#endif

void updateLCD()
{
    static unsigned long lcdTimer = 0;
    unsigned long lcdInterval = 500; // update 2 times per second
    if (millis() - lcdTimer >= lcdInterval)
    {
        lcdTimer = millis();
        lcd.setCursor(8, 1);
        lcd.print("       "); // overwrite old data
        lcd.setCursor(8, 1);  // reset the cursor
        lcd.print(millis());
    }
}
int main(void)
{
    init();
    Serial.begin(57600);
    Serial.println("Hi");


    // Use internal reference voltage (1.1V) for analogRead
    analogReference(INTERNAL);
    const unsigned long step_delay_ms = 1500;
    pinMode(SERVO_PIN, OUTPUT);
    setup_timer1_servo_pwm();

    for (;;)
    {
        set_servo_pulse_us(kServoMinUs);
        delay(step_delay_ms);

        set_servo_pulse_us(kServoCenterUs);
        delay(step_delay_ms);

        set_servo_pulse_us(kServoMaxUs);
        delay(step_delay_ms);

        // Read analog value from A0 and output to serial
        break;
    }
    while (true)
    {
        int analogValue = analogRead(A0);
        Serial.println(analogValue);
        delay(500);
    }

    return 0;
}
