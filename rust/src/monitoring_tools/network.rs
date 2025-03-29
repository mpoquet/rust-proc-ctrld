use std::net::TcpStream;
use std::net::TcpListener;
use netstat2::*;


pub fn is_port_open(add: &str, port: &str) -> bool {
    let target = format!("{}:{}", add, port);
    TcpStream::connect(target).is_ok()
}

pub fn open_port(addr: &str, port: &str) -> std::io::Result<TcpListener> {
    let target = format!("{}:{}", addr, port);
    let listener = TcpListener::bind(target)?;
    Ok(listener)
}

pub fn is_port_listening(port: u16) -> bool {
    let af_flags =
        AddressFamilyFlags::IPV4
        | AddressFamilyFlags::IPV6;
    let protocol_flags =
        ProtocolFlags::TCP
        | ProtocolFlags::UDP;
    let socket_info =
        get_sockets_info(af_flags, protocol_flags)
        .expect("error get_socket_info failed");

    for info in socket_info {
        match info.protocol_socket_info {
            ProtocolSocketInfo::Tcp(tcp_socket_info) => {
                return tcp_socket_info.local_port == port
                    && tcp_socket_info.state == TcpState::Listen;
            },
            ProtocolSocketInfo::Udp(_) => {
                return false;
            },
        }
    }
    false
}