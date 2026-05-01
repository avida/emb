#include <Arduino.h>

#define SERVO_PIN 9

static constexpr uint16_t kServoPeriodUs = 20000;
static constexpr uint16_t kServoMinUs = 1000;
static constexpr uint16_t kServoCenterUs = 1500;
static constexpr uint16_t kServoMaxUs = 2000;

static void setup_timer1_servo_pwm()
{
    // Timer1 Fast PWM, TOP=ICR1 (mode 14), non-inverting on OC1A (D9).
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    TCCR1A |= _BV(COM1A1);
    TCCR1A |= _BV(WGM11);
    TCCR1B |= _BV(WGM13) | _BV(WGM12);
    TCCR1B |= _BV(CS11); // Prescaler 8.

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
    pinMode(SERVO_PIN, OUTPUT);
    setup_timer1_servo_pwm();

    for (;;)
    {
        set_servo_pulse_us(kServoMinUs);
        Serial.println("Servo: 1000 us");
        delay(1000);

        set_servo_pulse_us(kServoCenterUs);
        Serial.println("Servo: 1500 us");
        delay(1000);

        set_servo_pulse_us(kServoMaxUs);
        Serial.println("Servo: 2000 us");
        delay(1000);
    }
    return 0;
}
