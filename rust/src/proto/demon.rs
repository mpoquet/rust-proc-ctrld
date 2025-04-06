use tokio::io::BufReader;
use tokio::net::TcpStream;
use tokio::sync::mpsc;

#[tokio::main]
async fn main() {
    let port: u16 = 8080;
    println!("We listen on the port {}", port);

    let tcp_stream = TcpStream::connect(format!("127.0.0.1:{}", port)).await.expect("error connecting tcp");
    let buffer = BufReader::new(tcp_stream);

    let (tx, mut rx) = mpsc::channel::<&str>(100);

    // tokio::spawn(
    //     // Logique de réception est d'envoi
    // );


    loop {
        if let Some(msg) = rx.recv().await {
            println!("got {}", msg);
        }
    }
}