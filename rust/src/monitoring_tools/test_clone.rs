use std::{thread, time};
mod clone;

fn main(){
    clone::setup_handler(clone::handle_sigchld);
    clone::execute_file(String::from("./foo"));

    thread::sleep(time::Duration::from_secs(5));
}