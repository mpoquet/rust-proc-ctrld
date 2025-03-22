use std::net::TcpStream;
use std::net::TcpListener;



pub fn is_port_open(add: &str, port: &str) -> bool {
    let target = format!("{}:{}", add, port);
    TcpStream::connect(target).is_ok()
}

pub fn open_socket(addr: &str) -> std::io::Result<TcpListener> {
    let listener = TcpListener::bind(addr)?;
    Ok(listener)
}