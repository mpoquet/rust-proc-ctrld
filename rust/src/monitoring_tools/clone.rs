use std::process::{Command, Child};
use std::sync::atomic::{AtomicI32, Ordering};
use nix::sys::signal::{self, SigHandler, Signal};
use nix::sys::wait::waitpid;
use nix::unistd::Pid;
use std::{thread, time};


// Variable atomique pour stocker le PID du fils
static CHILD_PID: AtomicI32 = AtomicI32::new(0);

extern "C" fn handle_sigchld(_: i32) {
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

fn main() {
    // Configuration du gestionnaire de signal
    unsafe {
        let handler = SigHandler::Handler(handle_sigchld);
        signal::signal(Signal::SIGCHLD, handler)
            .expect("Échec de l'enregistrement du gestionnaire SIGCHLD");
    }

    let child: Child = match Command::new("./foo").spawn() {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Erreur lors du lancement : {}", e);
            return;
        }
    };

    // Stockage atomique du PID
    let pid = child.id() as i32;
    CHILD_PID.store(pid, Ordering::Relaxed);

    println!("Processus lancé avec PID : {}", pid);

    // Attente active (à adapter selon les besoins)
    thread::sleep(time::Duration::from_secs(5));
}