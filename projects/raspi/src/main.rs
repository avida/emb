use rppal::gpio::{Gpio, Level, Trigger};
use std::env;
use std::error::Error;
use std::time::{SystemTime, UNIX_EPOCH};

const MAX_BCM_GPIO: u8 = 27;
const DEFAULT_INTERRUPT_PIN: u8 = 17;

fn level_to_str(level: Level) -> &'static str {
    match level {
        Level::Low => "LOW",
        Level::High => "HIGH",
    }
}

fn interrupt_pin_from_env() -> u8 {
    match env::var("GPIO_INTERRUPT_PIN") {
        Ok(value) => match value.parse::<u8>() {
            Ok(pin) => pin,
            Err(_) => {
                eprintln!(
                    "Invalid GPIO_INTERRUPT_PIN='{}'. Falling back to {}.",
                    value, DEFAULT_INTERRUPT_PIN
                );
                DEFAULT_INTERRUPT_PIN
            }
        },
        Err(_) => DEFAULT_INTERRUPT_PIN,
    }
}

fn dump_gpio_states(gpio: &Gpio) {
    println!("GPIO states (BCM 0..{MAX_BCM_GPIO}):");
    println!("{:<8} {:<10}", "GPIO", "VALUE");

    for pin_num in 0..=MAX_BCM_GPIO {
        match gpio.get(pin_num) {
            Ok(pin) => {
                let input = pin.into_input();
                println!("GPIO{pin_num:<4} {:<10}", level_to_str(input.read()));
            }
            Err(_) => {
                println!("GPIO{pin_num:<4} {:<10}", "N/A");
            }
        }
    }
}

fn main() -> Result<(), Box<dyn Error>> {
    let gpio = Gpio::new()?;

    dump_gpio_states(&gpio);

    let interrupt_pin = interrupt_pin_from_env();
    let mut pin = gpio.get(interrupt_pin)?.into_input();
    pin.set_interrupt(Trigger::Both)?;

    println!();
    println!(
        "Listening for GPIO{} edge changes (both rising and falling). Press Ctrl+C to stop.",
        interrupt_pin
    );

    loop {
        if let Some(level) = pin.poll_interrupt(true, None)? {
            let now = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap_or_default();
            println!(
                "[{}.{:03}] GPIO{} -> {}",
                now.as_secs(),
                now.subsec_millis(),
                interrupt_pin,
                level_to_str(level)
            );
        }
    }
}
