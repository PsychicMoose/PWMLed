use std::fs::File;
use std::io::{Read};
use std::thread;
use std::time::Duration;

fn main() {
    println!("Starting Device File Reader...");

    loop {
        // Open the device file
        let mut file = match File::open("/dev/project") {
            Ok(f) => f,
            Err(e) => {
                eprintln!("Failed to open /dev/project: {}", e);
                break;
            }
        };

        // Read speed
        let mut buffer = String::new();
        if let Err(e) = file.read_to_string(&mut buffer) {
            eprintln!("Failed to read: {}", e);
            break;
        }

        // Parse speed
        let speed = buffer.trim().parse::<i32>().unwrap_or(0);
        println!("Current speed: {}", speed);

        // Correct logic from your C driver
        if speed >= 8 {
            println!("Set LED1=100%, LED2=100%, LED3=100%");
        } else if speed >= 6 {
            println!("Set LED1=80%, LED2=70%, LED3=50%");
        } else if speed >= 4 {
            println!("Set LED1=60%, LED2=50%, LED3=30%");
        } else if speed >= 2 {
            println!("Set LED1=40%, LED2=30%, LED3=10%");
        } else {
            println!("Set LED1=10%, LED2=0%, LED3=0%");
        }

        thread::sleep(Duration::from_secs(2));
    }
}
