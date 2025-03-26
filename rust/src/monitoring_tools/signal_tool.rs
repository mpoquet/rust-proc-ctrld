use tokio::signal::unix::{signal, SignalKind};
use std::process;
use tokio::task;

pub async fn read_events_signal() -> Result<(), ()>{
    println!("PID = {}", process::id());
    let mut stream_signal = signal(SignalKind::interrupt())
        .expect("error assigning signal");

    task::spawn({
        async move {
            loop {
                stream_signal.recv().await;
                println!("GOT SIGINT");
            }
        }
    });
    Ok(())
}