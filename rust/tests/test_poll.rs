/**
 *  J'utilise dans un fichier a part car c'est une boucle infinie
 */

use rust_proc_ctrl::monitoring_tools::epoll::read_events_non_blocking;
use std::env;


// J'ai créer un bin pour ce test
#[tokio::main]
async fn main() {
    let args : Vec<String> = env::args().collect();
    if args.len() != 2 {
        println!("Usage : {} <file to watch>.", args[0]);
    }

    println!("We watch in the path : {}", args[1]);

    // Par exemple je met que je veux être notifié si la taille dépasse les 100 octets
    read_events_non_blocking(&args[1], 100)
        .await
        .expect("error read_event_non_blocking");

    loop {
        println!("Test de non bloquage");
        tokio::time::sleep(tokio::time::Duration::from_secs(2)).await;
    }
}