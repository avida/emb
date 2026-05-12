use rppal::gpio::{Gpio, Level, Trigger};
use std::env;
use std::error::Error;
use std::thread;
use std::thread::sleep;
use std::time::{Duration, SystemTime, UNIX_EPOCH};

extern crate nrf24l01;

use nrf24l01::{DataRate, OperatingMode, PALevel, TXConfig, NRF24L01};

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
    let config = TXConfig {
        data_rate: DataRate::R1Mbps,
        channel: 108,
        pa_level: PALevel::Low,
        pipe0_address: *b"abcde",
        max_retries: 3,
        retry_delay: 2,
        ..Default::default()
    };
    let mut device = NRF24L01::new(27, 0).unwrap();
    let message = b"sendtest";

    device.configure(&OperatingMode::TX(config)).unwrap();
    device.flush_output().unwrap();

    dump_gpio_states(&gpio);
    println!("GPIO dump done");

    let interrupt_pin = interrupt_pin_from_env();
    println!("Selected interrupt pin: GPIO{}", interrupt_pin);
    println!();
    let mut pin = gpio.get(interrupt_pin)?.into_input();
    pin.set_interrupt(Trigger::Both)?;

    println!();
    println!(
        "Listening for GPIO{} edge changes (both rising and falling). Press Ctrl+C to stop.",
        interrupt_pin
    );

    let handle = thread::spawn(move || {
        let mut pin = pin;
        loop {
            match pin.poll_interrupt(true, None) {
                Ok(Some(level)) => {
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
                Ok(None) => {}
                Err(e) => {
                    eprintln!("poll_interrupt error: {}", e);
                    break;
                }
            }
        }
    });

    let nrf_thread_handle = thread::spawn(move || {
        let mut device = device;
        loop {
            device.push(0, message).unwrap();
            match device.send() {
                Ok(retries) => println!("Message sent, {} retries needed", retries),
                Err(err) => {
                    println!("Destination unreachable: {:?}", err);
                    device.flush_output().unwrap();
                }
            };
            sleep(Duration::from_millis(2000));
        }
    });

    // Wait for the polling thread (will run until interrupted)
    handle.join().expect("GPIO polling thread panicked");
    nrf_thread_handle.join().expect("NRF thread panicked");

    Ok(())
}
