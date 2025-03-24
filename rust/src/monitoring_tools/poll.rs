use inotify::EventMask;
use nix::unistd::read;
use std::os::fd::RawFd;
use tokio::task;
use std::os::fd::AsRawFd;
use nix::libc::{self, epoll_create1, epoll_ctl, epoll_wait, epoll_event, fcntl, EPOLLIN, EPOLL_CTL_ADD, F_SETFL, O_NONBLOCK};

use crate::monitoring_tools::inotify_tool::*;

use super::signal_tool::create_signalfd;

fn set_non_blocking(fd: i32) -> Result<(), ()>{
    let flags = unsafe {
        libc::fcntl(fd, libc::F_GETFL)
    };

    if flags < 0 {
        return Err(());
    }
    if unsafe {fcntl(fd, F_SETFL, flags | O_NONBLOCK)} < 0 {
        return Err(());
    }

    Ok(())
}

pub fn add_fd_to_epoll(epoll_fd: i32, watch_fd: i32) -> Result<(), ()> {
    let mut event = epoll_event {
        events: EPOLLIN as u32,
        u64: watch_fd as u64,
    };

    let result = unsafe {
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, watch_fd, &mut event)
    };
    if result < 0 {
        return Err(());
    }
    Ok(())
}

// reach_size = size maximum of file for being notify
pub async fn read_events_non_blocking(path: &str, reach_size: u64) -> Result<(), ()>{
    let epoll_fd = unsafe {epoll_create1(0)};
    if epoll_fd < 0 {
        return Err(());
    }

    let mut inotify = create_inotify_watch_file(path);
    let inotify_fd = inotify.as_raw_fd();
    set_non_blocking(inotify_fd)
        .expect("error set_non_blocking() : inotify_fd failed.");
    add_fd_to_epoll(epoll_fd, inotify_fd)
        .expect("error failed to add inotifyfd to epoll");

    let signal_fd = create_signalfd().expect("error to create signalfd");
    let signal_raw_fd = signal_fd.as_raw_fd();
    add_fd_to_epoll(epoll_fd, signal_raw_fd)
        .expect("error failed to add signalfd to epoll");

    let mut events = [epoll_event { events: 0, u64: 0 }; 10];
    task::spawn({
            let path = path.to_string();
            async move {
            loop {
                let num_events = unsafe {
                    epoll_wait(epoll_fd, events.as_mut_ptr(), events.len() as i32, -1)
                };
                if num_events < 0 {
                    eprintln!("error num_events < 0");
                    break;
                }

                for event in &events[..num_events as usize] {
                    let fd = event.u64 as RawFd;

                    if fd == inotify_fd {
                        
                        let mut buffer = [0; 1024];
                        match inotify.read_events_blocking(&mut buffer) {
                            Ok(events) => {
                                for event in events {
                                    let event_mask = event.mask;
                                    let mut name_file = String::new();
        
                                    if let Some(name) = event.name {
                                        name_file = name.to_str()
                                            .expect("failed to convert to str")
                                            .to_string();
                                    }
        
                                    if event_mask.contains(EventMask::CLOSE_WRITE)
                                    || event_mask.contains(EventMask::CLOSE_NOWRITE) {
                                        let file_path = format!("{}/{}", path, name_file);
                                        let file_size = get_size_file(&file_path)
                                            .expect("error to get file size");
                                        
                                        if file_size > reach_size {
                                            println!("File {} reached size {} o.", name_file, file_size);
                                        }
                                    }
                                    if event_mask.contains(EventMask::CREATE) {
                                        print!("CREATE ");
                                        if event_mask.contains(EventMask::ISDIR) {
                                            print!("| DIR ");
                                        }
                                        println!("named = {}", name_file);
                                    }
                                }
                            }
                            Err(e) => {
                                eprintln!("Erreur lecture inotify: {}", e)
                            },
                        }
                    }
                    else if fd == signal_raw_fd {
                        let mut sig_buffer = [0u8; 128];
                        let res = read(signal_raw_fd, &mut sig_buffer);
                        match res {
                            Ok(_) => println!("Signal reçu"),
                            Err(e) => {
                                eprintln!("Erreur lors de la lecture du signalfd: {}", e);
                            },
                        }
                    }
                }
            }
        }
    });

    Ok(())
}