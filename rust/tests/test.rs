use std::sync::{atomic::{AtomicBool, Ordering}, Arc};
use std::thread;

use rust::network::start_server;

#[test]
fn test_server_start_stop() {
    let running = Arc::new(AtomicBool::new(true));
    let r = running.clone();

    let server_thread = thread::spawn(move || {
        start_server(r).unwrap();
    });

    // Simulate some work in the main thread
    thread::sleep(std::time::Duration::from_secs(1));

    // Stop the server
    running.store(false, Ordering::SeqCst);
    server_thread.join().unwrap();
}