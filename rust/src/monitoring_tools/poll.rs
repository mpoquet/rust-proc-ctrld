use inotify::Inotify;
use tokio::task;
use std::os::fd::AsRawFd;
use nix::libc::{self, epoll_create1, epoll_ctl, epoll_wait, epoll_event, fcntl, EPOLLIN, EPOLL_CTL_ADD, F_SETFL, O_NONBLOCK};

mod inotify_tool;


fn set_non_blocking(fd: i32) -> Result<(), ()>{
    let flags = unsafe {
        libc::fcntl(fd, libc::F_GETFL)
    };

    if flags < 0 {
        return Err(());
    }
    if unsafe { fcntl(fd, F_SETFL, flags | O_NONBLOCK)} < 0 {
        return Err(());
    }

    Ok(())
}

async fn read_events_non_blocking() -> Result<(), ()>{
    let mut inotify = Inotify::init().expect("error init inotify");
    let path = "/home/wer/Documents/rust-proc-ctrld/rust/src/monitoring_tools";
    
    inotify_tool::create_inotify_watch_file(&inotify, path);
    let inotify_fd = inotify.as_raw_fd();
    set_non_blocking(inotify_fd)
        .expect("error set_non_blocking() : inotify_fd failed.");

    let epoll_fd = unsafe {epoll_create1(0)};

    if epoll_fd < 0 {
        return Err(());
    }

    // Ajouter inotify à epoll
    let mut event = epoll_event {
        events: EPOLLIN as u32,
        u64: epoll_fd as u64,
    };

    let result = unsafe {
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, inotify_fd, &mut event)
    };

    if result < 0 {
        return Err(());
    }

    let mut events = [epoll_event { events: 0, u64: 0 }; 10];

    task::spawn(async move {
        loop {
            let num_events = unsafe { epoll_wait(epoll_fd, events.as_mut_ptr(), events.len() as i32, -1) };
            if num_events < 0 {
                eprintln!("error num_events < 0");
                break;
            }

            let mut buffer = [0; 1024];
            match inotify.read_events_blocking(&mut buffer) {
                Ok(events) => {
                    for event in events {
                        let event_mask = inotify_tool::DisplayEventMask(event.mask);

                        match event.name {
                            Some(name) => println!("Event = {}, for the file : {:?}.", event_mask, name),
                            None => {},
                        } 
                    }
                }
                Err(e) => {
                    eprintln!("Erreur lecture inotify: {}", e)
                },
            }
        }
    });

    Ok(())
}

#[tokio::main]
async fn main() {
    read_events_non_blocking()
        .await
        .expect("error read_event_non_blocking");

    loop {
        println!("Test de non blockage");
        tokio::time::sleep(tokio::time::Duration::from_secs(2)).await;
    }
}