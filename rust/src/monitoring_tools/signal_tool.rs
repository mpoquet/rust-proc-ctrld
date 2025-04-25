use nix::errno::Errno;
use nix::sys::signalfd::SignalFd;
use nix::sys::signal::{sigprocmask, SigSet, SigmaskHow, Signal};
use nix::unistd::Pid;
use std::os::unix::io::RawFd;
use std::io;

use std::process;
use nix::libc::{self, epoll_ctl, epoll_event, EPOLLIN, EPOLL_CTL_ADD};
use tokio::signal::unix::signal;
use tokio::task;
use tokio::signal::unix::SignalKind;

// For SignalHook
use signal_hook::{consts::SIGINT, iterator::Signals};
use std::{error::Error, thread};

// Using channel (crossbeam-channel = "0.5")
use crossbeam_channel::{bounded, tick, Receiver, select};



enum TypeFd {
    SIGNALFD = 1,
    INOTIFYFD = 2,
}

pub struct EventData {
    fd: RawFd,
    event_type: TypeFd,
}


pub fn create_signalfd() -> io::Result<SignalFd> {
    let mut mask = SigSet::empty();
    
    mask.add(Signal::SIGINT);

    mask.thread_block().expect("error blocking signal");

    Ok(SignalFd::new(&mask).expect("error making instance of SignalFd"))
}


pub fn add_signalfd_to_epoll(fd: i32,epoll_fd: i32) {
    let edata = Box::new(EventData {
        fd,
        event_type: TypeFd::SIGNALFD,
    });
    
    let edata_ptr = Box::into_raw(edata) as u64;
    
    let mut ev = epoll_event {
        events: EPOLLIN as u32,
        u64: edata_ptr,
    };
    
    let res = unsafe {epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &mut ev)};
    if res == -1 {
        panic!("error epoll_ctl return -1");
    }
}

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