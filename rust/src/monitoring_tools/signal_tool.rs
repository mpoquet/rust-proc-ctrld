use nix::libc::{epoll_ctl, epoll_event, epoll_wait, EPOLLIN, EPOLL_CTL_ADD};
use std::io;
use std::sync::Arc;
use tokio::signal::unix::{signal, SignalKind};
use nix::unistd::read;
use std::os::fd::{AsRawFd, RawFd};
use nix::sys::signalfd::{SignalFd, SigSet, SfdFlags};
use nix::sys::signal::{Signal, SigmaskHow, pthread_sigmask};
use std::process;
use tokio::task;

use crate::monitoring_tools::epoll::{self, *};

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

    nix::sys::signal::pthread_sigmask(nix::sys::signal::SigmaskHow::SIG_BLOCK, Some(&mask), None)
        .map_err(|_| io::Error::new(io::ErrorKind::Other, "Failed to block SIGINT"))?;

    SignalFd::with_flags(&mask, SfdFlags::SFD_NONBLOCK)
        .map_err(|_| io::Error::new(io::ErrorKind::Other, "Failed to create signalfd"))
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

pub async fn read_events_signal_epoll() -> Result<(), ()>{
    println!("PID = {}", process::id());

    let epoll_fd = create_epoll().expect("error creating epoll file descriptor");
    let signal_fd = create_signalfd().expect("error creating signal file descriptor");

    println!("Signalfd created: fd = {}", signal_fd.as_raw_fd());
    let signal_raw_fd = signal_fd.as_raw_fd();
    add_signalfd_to_epoll(signal_raw_fd, epoll_fd);

    let mut events = [epoll_event_empty(); 10];

    task::spawn({
        async move {
            loop {
                let num_events = unsafe {
                    epoll_wait(epoll_fd, events.as_mut_ptr(), 10, -1)
                };
                println!("here");
                    
                for i in 0..num_events as usize {
                    if events[i].u64 == signal_raw_fd as u64 {
                        let mut buffer = [0u8; 128];
                        let res = read(signal_raw_fd, &mut buffer);
                        match res {
                            Ok(_) => println!("GOT SIGINT"),
                            Err(_) => {
                                eprintln!("failed to read signal file descriptor");
                                break
                            }
                        }
                    }
                }
            }
        }
    });

    Ok(())
}