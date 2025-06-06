use std::fs::metadata;
use inotify::EventMask;
use nix::{errno::Errno, libc::{epoll_event, epoll_wait}};
use tokio::task;
use std::os::fd::AsRawFd;
use inotify::{
    Inotify, WatchMask
};

use super::epoll::*;

pub fn create_inotify_watch_file(path: &str) -> Inotify {
    let inotify = Inotify::init()
        .expect("error inotify init.");

    inotify.watches()
        .add(
            path,
            WatchMask::CREATE
            | WatchMask::CLOSE_NOWRITE
            | WatchMask::CLOSE_WRITE
            | WatchMask::DELETE
            | WatchMask::ACCESS
            | WatchMask::MODIFY
        )
        .expect("error adding file watch");

    inotify
}

pub fn inotify_read_blocking(inotify: &mut Inotify) {
    let mut buffer = [0; 1024];

    let events = (*inotify).read_events_blocking(&mut buffer)
        .expect("error while reading events in buffer");

    for event in events {
        let event_mask = event.mask;

        match event.name {
            Some(name) => println!("Event = {:?}, for the file : {:?}.", event_mask, name),
            None => println!("No event record"),
        }        
    }
}

pub fn get_size_file(path: &String) -> std::io::Result<u64> {
    let meta_data = metadata(path)?;
    let file_size = meta_data.len();
    Ok(file_size)
}

// reach_size = size maximum of file for being notify
pub async fn read_events_inotify(path: &str, vec: Vec<EventMask>, reach_size: u32) -> Result<u32, (u32,Errno)>{
    let epoll_fd = create_epoll()
    .expect("error creating epoll_fd");


let mut inotify = create_inotify_watch_file(path);
let inotify_fd = inotify.as_raw_fd();
set_non_blocking(inotify_fd)
.expect("error set_non_blocking() : inotify_fd failed.");
add_fd_to_epoll(epoll_fd, inotify_fd)
.expect("error failed to add inotifyfd to epoll");

let mut events = [epoll_event { events: 0, u64: 0 }; 10];
let handle = task::spawn({

    let path = path.to_string();

    async move {
        loop {
            let num_events = unsafe {
                epoll_wait(epoll_fd, events.as_mut_ptr(), events.len() as i32, -1)
            };
            if num_events < 0 {
                eprintln!("error num_events < 0");
                return Err((std::process::id(), Errno::UnknownErrno));
            }
            
            let mut buffer = [0; 1024];
            match inotify.read_events_blocking(&mut buffer) {
                Ok(events) => {
                    for event in events {

                        let event_mask = event.mask;
                        let mut name_file = path.to_string();
                        
                        if let Some(name) = event.name {
                            println!("read events");
                            name_file = name.to_str()
                                .expect("failed to convert to str")
                                .to_string();

                        }

                        let file_path =
                            if metadata(&path).map(|md| md.is_dir()).unwrap_or(false) {
                                format!("{}/{}", path, name_file)
                            } else {
                                name_file.clone()
                        };

                        let file_size = get_size_file(&file_path)
                            .expect("error to get file size") as u32;

                        if vec.iter().any(|&mask| event_mask.contains(mask)) {
                            if event_mask.contains(EventMask::CLOSE_WRITE)
                            || event_mask.contains(EventMask::CLOSE_NOWRITE) {
                                
                                if file_size > reach_size {
                                    println!("File {} reached size {} o.", name_file, file_size);
                                    break;
                                }
                            }
                            if event_mask.contains(EventMask::CREATE) && name_file != "" {
                                print!("CREATE ");
                                if event_mask.contains(EventMask::ISDIR) {
                                    print!("| DIR ");
                                }
                                println!("named = {}", name_file);
                                break;
                            }
                            if event_mask.contains(EventMask::ACCESS) && name_file != "" {
                                print!("ACCESS ");
                                println!("named = {}", name_file);
                                break;
                            }
                            if event_mask.contains(EventMask::DELETE) && name_file != "" {
                                print!("DELETE ");
                                println!("named = {}", name_file);
                                break;
                            }
                            if event_mask.contains(EventMask::MODIFY) && name_file != "" {
                                print!("MODIFY");
                                println!("named = {}", name_file);
                                break;
                            }
                        }
                    }
                }
                Err(e) => {
                    let errno = Errno::from_raw(e.raw_os_error().unwrap_or(0));
                    return Err((std::process::id(), errno));
                },
            } // end match
            return Ok(std::process::id())
        }
        }
    });
    
    handle.await.expect("error handle")
}