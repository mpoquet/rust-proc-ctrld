/**
 *  Test in infinite loop
 */

use rust_proc_ctrl::monitoring_tools::signal_tool::*;


// Bin created in Cargo.toml
#[tokio::main]
async fn main() {
    // Will send a message on ctrl+c
    read_events_signal_sighook().await.expect("error reading events signals");

    loop {
        println!("Test de non bloquage");
        tokio::time::sleep(tokio::time::Duration::from_secs(2)).await;
    }
}