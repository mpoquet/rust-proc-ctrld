use nix::errno::Errno;
use nix::sys::signal::Signal;
use nix::unistd::Pid;

use std::process;
use tokio::signal::unix::signal;
use tokio::task;
use tokio::signal::unix::SignalKind;

// For SignalHook
use signal_hook::{consts::SIGINT, iterator::Signals};
use std::{error::Error, thread};

// Using channel (crossbeam-channel = "0.5")
use crossbeam_channel::{bounded, Receiver};

pub async fn read_events_signal_tokio() -> Result<(), ()>{
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

pub async fn read_events_signal_ctrlc() -> Result<(), ctrlc::Error>{
    println!("PID = {}", process::id());
    ctrlc::set_handler(|| {
        println!("GOT SIGINT")
    })
}

pub async fn read_events_signal_sighook() -> Result<(), Box<dyn Error>> {
    let mut signals = Signals::new([SIGINT])?;

    thread::spawn(move || {
        println!("PID = {}", process::id());
        for sig in signals.forever() {
            println!("Receive signal {:?}", sig);
        }
    });

    Ok(())
}


pub fn ctrl_channel() -> Result<Receiver<()>, ctrlc::Error> {
    let (sender, receiver) = bounded(100);
    ctrlc::set_handler(move || {
        let _ = sender.send(());
    })?;

    Ok(receiver)
}

pub fn send_sigkill(pid : i32) -> Result<u32, (u32,Errno)> {
    let pid = Pid::from_raw(pid);
    if let Err(e) = nix::sys::signal::kill(pid, Signal::SIGKILL) {
        return Err((std::process::id(), e));
    }
    Ok(std::process::id())
}