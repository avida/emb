#include <Arduino.h>

#define SERVO_PIN 9

static constexpr uint16_t kServoMinUs = 500;
static constexpr uint16_t kServoCenterUs = 1500;
static constexpr uint16_t kServoMaxUs = 2500;


static void setup_timer1_servo_pwm()
{
    // Timer1 Fast PWM, TOP=ICR1 (mode 14), non-inverting on OC1A (D9).
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    TCCR1A |= _BV(COM1A1);
    TCCR1A |= _BV(WGM11);
    TCCR1B |= _BV(WGM13) | _BV(WGM12);
    TCCR1B |= _BV(CS11);

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

int main(void)
{
    init();

    Serial.begin(57600);
    Serial.println("default project boot");

    analogReference(INTERNAL);
    pinMode(SERVO_PIN, OUTPUT);
    setup_timer1_servo_pwm();

    for (;;)
    {
        set_servo_pulse_us(kServoMinUs);
        delay(1000);
        set_servo_pulse_us(kServoCenterUs);
        delay(1000);
        set_servo_pulse_us(kServoMaxUs);
        delay(1000);

        Serial.println(analogRead(A0));
    }

    return 0;
}
