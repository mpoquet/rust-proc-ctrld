use tokio::io::{AsyncReadExt, AsyncWriteExt, Interest};
use tokio::net::TcpStream;
use std::time;
use nix::sys::ptrace;
use nix::sys::wait::{waitpid, WaitStatus};
use nix::unistd::Pid;
use nix::libc::{SYS_socket, SYS_connect, SYS_bind, SYS_listen};
use std::process::Command;
use std::os::unix::process::CommandExt;
use std::time::Duration;
use tokio::task;
use nix::sys::signal::Signal;

fn trace_process_sync(pid: Pid) {
    println!("On attend un changement d'état du process...");
    match waitpid(pid, None) {
        Ok(WaitStatus::Stopped(_, _)) => println!("Processus arrêté, prêt pour le traçage"),
        _ => {
            eprintln!("Le processus n'est pas dans l'état attendu");
            return;
        }
    }

    loop {
        let status = match waitpid(pid, None) {
            Ok(s) => s,
            Err(e) => {
                eprintln!("Erreur lors de l'attente du processus {}: {}", pid, e);
                break;
            }
        };

        match status {
            WaitStatus::PtraceSyscall(_) => {
                if let Ok(regs) = ptrace::getregs(pid) {
                    let syscall_number = regs.orig_rax;
                    if syscall_number != u64::MAX { // Entrée syscall
                        match syscall_number as i64 {
                            SYS_socket => println!("[PID {}] socket() détecté !", pid),
                            SYS_connect => println!("[PID {}] connect() détecté !", pid),
                            SYS_bind => println!("[PID {}] bind() détecté !", pid),
                            SYS_listen => {
                                println!("[PID {}] listen() détecté !", pid);
                                // Détacher après avoir détecté listen()
                                ptrace::detach(pid,None).expect("Détachement échoué");
                                break;
                            },
                            _ => {},
                        }
                    }
                }
                ptrace::syscall(pid, None).expect("Échec de la reprise");
            },
            WaitStatus::Exited(pid_, status) => {
                println!("Processus {} terminé avec statut {}", pid_, status);
                break;
            },
            WaitStatus::Signaled(pid_, signal, _) => {
                println!("Processus {} tué par {}", pid_, signal);
                break;
            },
            WaitStatus::Stopped(pid_, signal) => {
                println!("Processus {} arrêté par {:?}", pid_, signal);
                ptrace::syscall(pid, None).expect("Échec de la reprise");
            },
            _ => {
                println!("État inattendu: {:?}", status);
                break;
            }
        }
    }
}

unsafe fn start_target_process(command: &str) -> Pid {
    let mut child = Command::new(command)
        .spawn()
        .unwrap_or_else(|e| panic!("Erreur d'exécution : {}", e));

    let pid = Pid::from_raw(child.id() as i32);
    println!("Processus lancé avec PID : {}", pid);

    ptrace::attach(pid).unwrap_or_else(|e| {
        panic!("Échec de l'attachement ptrace: {}", e);
    });

    waitpid(pid, None).expect("Échec de l'attente initiale");
    pid
}

async unsafe fn launch_trace_process(target: &str) -> Result<(), Box<dyn std::error::Error>> {
    let pid = start_target_process(target);
    
    ptrace::setoptions(
        pid,
        nix::sys::ptrace::Options::PTRACE_O_TRACESYSGOOD
            | nix::sys::ptrace::Options::PTRACE_O_TRACECLONE
            | nix::sys::ptrace::Options::PTRACE_O_TRACEFORK
            | nix::sys::ptrace::Options::PTRACE_O_TRACEVFORK,
    )?;

    ptrace::syscall(pid, None)?;

    task::spawn_blocking(move || {
        trace_process_sync(pid);
    });

    Ok(())
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    unsafe {
        launch_trace_process("/home/telmo/Bureau/Bureau d'étude/Code/rust-proc-ctrld/rust/target/debug/Start_TCP_socket").await?;
    }

    // Attendre que le serveur soit prêt
    tokio::time::sleep(Duration::from_secs(1)).await;

    // Connexion avec réessais
    let mut stream;
    let mut attempts = 0;
    loop {
        match TcpStream::connect("127.0.0.1:46550").await {
            Ok(s) => {
                stream = s;
                break;
            },
            Err(e) => {
                if attempts >= 20 {
                    return Err(e.into());
                }
                let delay = std::cmp::min(100 * 2u64.pow(attempts), 5000);
                eprintln!("Nouvelle tentative dans {}ms...", delay);
                tokio::time::sleep(Duration::from_millis(delay)).await;
                attempts += 1;
            }
        }
    }

    let addr = stream.local_addr();

    let (mut read_half, mut write_half) = stream.into_split();

    tokio::spawn(async move {
        loop {
            println!("En attente d'une notification");
            if let Err(e) = read_half.ready(Interest::READABLE).await {
                eprintln!("Erreur lors de l'attente de READABLE : {}", e);
                return;
            }
            let mut buf = [0u8; 1024];
            match read_half.read(&mut buf).await {
                Ok(0) => {
                    println!("Fermeture de la connexion de {:?}", addr.ok());
                    return;
                }
                Ok(n) => {
                    println!("Reçu {} octets de {:?}: {:?}", n, addr, &buf[..n]);
                    println!("Fermeture de la connexion de {:?}", addr);
                    break;
                }
                Err(e) => {
                    eprintln!("Erreur de lecture sur {:?}: {}", addr, e);
                    break;
                }
            }
        }
    });

    let message = b"Bonjour, socket!";
    write_half.write_all(message).await?;
    println!("Message envoyé !");

    loop {
        tokio::select! {
            _ = tokio::time::sleep(time::Duration::from_secs(15)) => {
                println!("Fin du programme.");
                break;
            }
        }
    }

    Ok(())
}