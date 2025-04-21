use inotify::{Event, EventMask};
/**
 *  Tests with infinite loop (ctrl+c) to end it.
 */

use rust_proc_ctrl::monitoring_tools::inotify_tool::read_events_inotify;
use std::env;


// Bin created in Cargo.toml
#[tokio::main]
async fn main() {
    let args : Vec<String> = env::args().collect();
    if args.len() != 2 {
        println!("Usage : {} <file to watch>.", args[0]);
    }

    println!("We watch in the path : {}", args[1]);

    // Notify when create dir / file in args[1] and file reach size of 5000 in the same dir
    let trigger = vec![EventMask::CREATE, EventMask::ACCESS, EventMask::DELETE];
    read_events_inotify(&args[1], trigger, 5000)
        .await
        .expect("error read_event_non_blocking");

    loop {
        println!("Test de non bloquage");
        tokio::time::sleep(tokio::time::Duration::from_secs(2)).await;
    }
}