use std::process::Command;
use std::sync::atomic::{AtomicI32, Ordering};
use nix::sys::signal::{self, SigHandler, Signal};
use nix::sys::wait::waitpid;
use nix::unistd::Pid;
use std::{thread, time};


// Variable atomique pour stocker le PID du fils
static CHILD_PID: AtomicI32 = AtomicI32::new(0);

pub extern "C" fn handle_sigchld(_: i32) {
    // Récupère le PID de manière atomique
    let raw_pid = CHILD_PID.load(Ordering::Relaxed);
    if raw_pid != 0 {
        let pid = Pid::from_raw(raw_pid);
        match waitpid(pid, None){
            Err(e) => println!("Erreur lors de l'attente du fils : {}", e),
            Ok(t) => println!("Le fils s'est terminé avec le status {:?}", t),
        }

    }
}

pub fn execute_file(path : String){
    println!("avant le spawn");
    match Command::new(path).spawn() {
        Ok(c) => {
            println!("Processus lancé avec PID : {}", c.id());
            CHILD_PID.store(c.id() as i32, Ordering::Relaxed);
        },
        Err(e) => println!("Error while trying to execute file : {}", e),
    }
}

pub fn setup_handler(f_handler: extern "C" fn(i32)){
    unsafe {
        let handler = SigHandler::Handler(f_handler);
        match signal::signal(Signal::SIGCHLD, handler){
            Ok(t) => println!("Handler has been succesfully placed : {:?}", t),
            Err(e) => println!("Error while trying to place the handler {}", e),
        }
    }
}

pub fn main() {
    // Configuration du gestionnaire de signal
    setup_handler(handle_sigchld);

    execute_file(String::from("./foo"));

    // Attente active (à adapter selon les besoins)
    thread::sleep(time::Duration::from_secs(2));
}