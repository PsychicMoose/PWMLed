use std::fs::{File, OpenOptions};
use std::io::{Read, Write};
use std::thread;
use std::time::Duration;

fn read_speed() -> i32 {
    let mut file = match File::open("/sys/class/project/project/speed") {
        Ok(f) => f,
        Err(e) => {
            eprintln!("Failed to open speed file: {}", e);
            return 0;
        }
    };

    let mut buffer = String::new();
    if let Err(e) = file.read_to_string(&mut buffer) {
        eprintln!("Failed to read speed: {}", e);
        return 0;
    }

    buffer.trim().parse::<i32>().unwrap_or(0)
}

fn write_duty_cycle(led: &str, duty: i32) {
    let path = format!("/sys/class/project/project/{}", led);
    let mut file = match OpenOptions::new().write(true).open(&path) {
        Ok(f) => f,
        Err(e) => {
            eprintln!("Failed to open {}: {}", path, e);
            return;
        }
    };

    if let Err(e) = writeln!(file, "{}", duty) {
        eprintln!("Failed to write to {}: {}", path, e);
    }
}

fn main() {
    println!("Starting Sysfs Controller...");

    loop {
        let speed = read_speed();
        println!("Current speed: {}", speed);

        // Match C driver logic exactly
        if speed >= 8 {
            write_duty_cycle("led1_duty", 100);
            write_duty_cycle("led2_duty", 100);
            write_duty_cycle("led3_duty", 100);
        } else if speed >= 6 {
            write_duty_cycle("led1_duty", 80);
            write_duty_cycle("led2_duty", 70);
            write_duty_cycle("led3_duty", 50);
        } else if speed >= 4 {
            write_duty_cycle("led1_duty", 60);
            write_duty_cycle("led2_duty", 50);
            write_duty_cycle("led3_duty", 30);
        } else if speed >= 2 {
            write_duty_cycle("led1_duty", 40);
            write_duty_cycle("led2_duty", 30);
            write_duty_cycle("led3_duty", 10);
        } else {
            write_duty_cycle("led1_duty", 10);
            write_duty_cycle("led2_duty", 0);
            write_duty_cycle("led3_duty", 0);
        }

        thread::sleep(Duration::from_secs(2));
    }
}
