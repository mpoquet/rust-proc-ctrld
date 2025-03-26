/**
 *  Test in infinite loop
 */

use rust_proc_ctrl::monitoring_tools::signal_tool::read_events_signal;


// Bin created in Cargo.toml
#[tokio::main]
async fn main() {
    // Will send a message on ctrl+c
    read_events_signal().await.expect("error reading events signals");

    loop {
        println!("Test de non bloquage");
        tokio::time::sleep(tokio::time::Duration::from_secs(2)).await;
    }
}