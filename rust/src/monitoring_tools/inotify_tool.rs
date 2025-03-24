use std::fs::metadata;
use inotify::{
    Inotify, WatchMask
};

pub fn create_inotify_watch_file(path: &str) -> Inotify {
    let inotify = Inotify::init()
        .expect("error inotify init.");

    inotify.watches()
        .add(
            path,
            WatchMask::CREATE
            | WatchMask::CLOSE_NOWRITE
            | WatchMask::CLOSE_WRITE
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