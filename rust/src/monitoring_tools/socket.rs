use std::net::TcpStream;

use std::net::TcpListener;



pub fn is_socket_open(add: &str) -> bool {
    TcpStream::connect(add).is_ok()
}

pub fn open_socket(addr: &str) -> std::io::Result<TcpListener> {
    let listener = TcpListener::bind(addr)?;
    Ok(listener)
}