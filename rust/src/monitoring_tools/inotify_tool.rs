use std::fmt;

use inotify::{
    EventMask, Inotify, WatchMask
};

#[derive(Debug)]
pub struct DisplayEventMask(pub EventMask);

pub fn create_inotify_watch_file(inotify: &Inotify, path: &str) {
    inotify.watches()
        .add(
            path,
            WatchMask::ALL_EVENTS
        )
        .expect("error adding file watch");
}

pub fn inotify_read_blocking(inotify: &mut Inotify) {
    let mut buffer = [0; 1024];

    let events = (*inotify).read_events_blocking(&mut buffer)
        .expect("error while reading events in buffer");

    for event in events {
        let event_mask = DisplayEventMask(event.mask);

        match event.name {
            Some(name) => println!("Event = {}, for the file : {:?}.", event_mask, name),
            None => println!("No event record"),
        }        
    }
}

impl fmt::Display for DisplayEventMask {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let mut events : Vec<&str> = Vec::new();

        match self.0 {
            EventMask::ACCESS => events.push("IN_ACCESS"),
            EventMask::ATTRIB => events.push("IN_ATTRIB"),
            EventMask::CLOSE_NOWRITE => events.push("IN_CLOSE_NOWRITE"),
            EventMask::CLOSE_WRITE => events.push("IN_CLOSE_WRITE"),
            EventMask::CREATE => events.push("IN_CREATE"),
            EventMask::DELETE => events.push("IN_DELETE"),
            EventMask::DELETE_SELF => events.push("IN_DELETE_SELF"),
            EventMask::IGNORED => events.push("IN_IGNORED"),
            EventMask::ISDIR => events.push("IN_ISDIR"),
            EventMask::MODIFY => events.push("IN_MODIFY"),
            EventMask::MOVE_SELF => events.push("IN_MOVE_SELF"),
            EventMask::MOVED_FROM => events.push("IN_MOVE_FROM"),
            EventMask::MOVED_TO => events.push("IN_MOVE_TO"),
            EventMask::OPEN => events.push("IN_OPEN"),
            EventMask::Q_OVERFLOW=> events.push("IN_Q_OVERFLOW"),
            EventMask::UNMOUNT=> events.push("IN_UNMOUNT"),
            _ => events.push("OTHER"),
        }


        write!(f, "{}", events.join(" | "))
    }
}


fn main() {
    let mut inotify = Inotify::init().expect("error inotify init.");
    create_inotify_watch_file(&inotify, "/home/wer/Documents/rust-proc-ctrld/rust/src/monitoring_tools");

    inotify_read_blocking(&mut inotify);

    inotify.close().expect("error closing inotify instance");
}