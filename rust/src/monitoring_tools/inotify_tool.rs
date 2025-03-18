use inotify::{
    EventMask, Inotify, WatchMask
};

pub fn create_inotify_watch_file(path: &str) -> Inotify {
    let inotify = Inotify::init()
        .expect("error inotify init.");

    inotify.watches()
        .add(
            path,
            WatchMask::ALL_EVENTS
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

pub fn print_event(event: EventMask, source: &str) {

    if event.contains(EventMask::CREATE) {
        print!("CREATE ");
        if event.contains(EventMask::ISDIR) {
            print!("| DIR ");
        }
        else {
            print!("| FILE ")
        }
        println!("named : {}", source);
    }

}

fn main() {
    ()
}