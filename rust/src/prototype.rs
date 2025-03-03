use std::{fs::{self, File}, process::Command};
use daemonize::Daemonize;

/*
    Je créer un processus (daemon) en arrière plan
    Je lui redirige l'entrée et la sortie de celui-ci (/tmp)
    J'écris le pid dans un fichier tmp
*/
pub fn create_daemon() {
    // Vérifier et créer le répertoire tmp s'il n'existe pas
    let tmp_dir = "tmp";
    if !fs::metadata(tmp_dir).is_ok() {
        fs::create_dir(tmp_dir)
            .expect("Failed to create tmp directory");
    }

    // Supprimer le fichier PID s'il existe déjà
    let pid_file = "tmp/daemon.pid";
    if fs::metadata(pid_file).is_ok() {
        fs::remove_file(pid_file)
            .expect("Failed to remove existing PID file");
    }

    let stdout = File::create("tmp/daemon.out").unwrap();
    let stderr = File::create("tmp/daemon.err").unwrap();

    let daemon = Daemonize::new()
        .stdout(stdout)
        .stderr(stderr)
        .pid_file("tmp/daemon.pid");

    match daemon.start() {
        Ok(_) => {
            println!("Daemon lancé !");
            let output = Command::new("ls").output().unwrap();

            println!("{}", String::from_utf8_lossy(&output.stdout));
            stop_daemon();
        }
        Err(e) => eprintln!("Erreur : {}", e),
    }
}

// Fonction qui lit dans /tmp/daemon.pid et kill le programme
pub fn stop_daemon() {
    // Lire le fichier PID
    let pid_file = "/tmp/daemon.pid";
    if let Ok(pid) = fs::read_to_string(pid_file) {
        let pid = pid.trim();
        // Utiliser la commande kill pour arrêter le processus
        let output = Command::new("kill")
            .arg(pid)
            .output()
            .expect("Failed to execute kill command");

        if output.status.success() {
            println!("Daemon arrêté avec succès");
            // Supprimer le fichier PID après avoir arrêté le daemon
            fs::remove_file(pid_file).expect("Failed to remove PID file");
        } else {
            eprintln!("Erreur lors de l'arrêt du daemon : {}", String::from_utf8_lossy(&output.stderr));
        }
    } else {
        eprintln!("Erreur : impossible de lire le fichier PID");
    }
}