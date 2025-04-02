/**
 *  Test in infinite loop
 */

use rust_proc_ctrl::monitoring_tools::network::*;


// Bin created in Cargo.toml
#[tokio::main]
async fn main() {
    // Will send a message on ctrl+c
    let port: u16 = 8080;
    read_events_port_tokio(port).await.expect("error reading events port tokio");
    read_events_port_scanner(port).await.expect("error reading events port scanner");

    loop {
        println!("Test de non bloquage");
        tokio::time::sleep(tokio::time::Duration::from_secs(2)).await;
    }
}