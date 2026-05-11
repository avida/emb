## ATmega (Arduino Uno) port-to-pin mapping

Below is a compact mapping for the ATmega328P (Arduino Uno) showing MCU port pins and their corresponding Arduino pin numbers.

```
PORTD (digital 0..7)
 PD0  -> D0  (RX)
 PD1  -> D1  (TX)
 PD2  -> D2
 PD3  -> D3  (PWM)
 PD4  -> D4
 PD5  -> D5  (PWM)
 PD6  -> D6  (PWM)
 PD7  -> D7

PORTB (digital 8..13)
 PB0  -> D8
 PB1  -> D9  (PWM)
 PB2  -> D10 (PWM, SS)
 PB3  -> D11 (MOSI, PWM)
 PB4  -> D12 (MISO)
 PB5  -> D13 (SCK)
 PB6  -> XTAL1 / not used as a standard Arduino Digital pin on Uno
 PB7  -> XTAL2 / not used as a standard Arduino Digital pin on Uno

PORTC (analog A0..A5)
 PC0  -> A0
 PC1  -> A1
 PC2  -> A2
 PC3  -> A3
 PC4  -> A4 (SDA)
 PC5  -> A5 (SCL)
 PC6  -> RESET (not an analog input)

Notes:
 - This mapping is for the common ATmega328P used on the Arduino Uno. Other ATmega models (e.g., ATmega2560) have different mappings.
 - PWM-capable pins are annotated. SPI/I2C pins are noted where applicable.
 - PB6/PB7 on the ATmega328P are typically used for the clock crystal on Uno boards and are not exposed as the standard Arduino D pins.
```

# Raspberry Pi 40-pin header (ASCII pinout)

Below is a compact ASCII representation of the Raspberry Pi 40-pin header showing physical pins, common names and BCM numbers.
+-----+-------------------------+-------------------------+-----+
| Pin | Left (Name / BCM)      | Right (Name / BCM)     | Pin |
+-----+-------------------------+-------------------------+-----+
|  1  | 3.3V                   | 5V                      |  2  |
|  3  | GPIO2  (SDA, BCM2)     | 5V                      |  4  |
|  5  | GPIO3  (SCL, BCM3)     | GND                     |  6  |
|  7  | GPIO4  (GPCLK0, BCM4)  | TXD0 (GPIO14, BCM14)    |  8  |
|  9  | GND                    | RXD0 (GPIO15, BCM15)    | 10  |
| 11  | GPIO17                 | GPIO18                  | 12  |
| 13  | GPIO27                 | GND                     | 14  |
| 15  | GPIO22                 | GPIO23                  | 16  |
| 17  | 3.3V                   | GPIO24                  | 18  |
| 19  | MOSI (GPIO10, BCM10)   | GND                     | 20  |
| 21  | MISO (GPIO9, BCM9)     | GPIO25                  | 22  |
| 23  | SCLK (GPIO11, BCM11)   | CE0 (GPIO8, BCM8)       | 24  |
| 25  | GND                    | CE1 (GPIO7, BCM7)       | 26  |
| 27  | ID_SD (GPIO0) EEPROM   | ID_SC (GPIO1) EEPROM    | 28  |
| 29  | GPIO5                  | GND                     | 30  |
| 31  | GPIO6                  | GPIO12                  | 32  |
| 33  | GPIO13                 | GND                     | 34  |
| 35  | GPIO19                 | GPIO16                  | 36  |
| 37  | GPIO26                 | GPIO20                  | 38  |
| 39  | GND                    | GPIO21                  | 40  |
+-----+-------------------------+-------------------------+-----+

Notes:
- Pins shown by physical number (1–40). Left column lists the odd pins; right column the even pins.
- BCM numbers shown where common (e.g., `BCM2` = `GPIO2`).
- `ID_SD` / `ID_SC` (pins 27/28) are used for HAT EEPROM on some models.

```
