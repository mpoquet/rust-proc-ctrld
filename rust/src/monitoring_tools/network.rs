use std::time::Duration;
use netstat2::*;
use nix::errno::Errno;
use tokio::task;
use std::net::{TcpStream, TcpListener};

// Port scanner
use port_scanner::scan_port;

pub fn is_port_open(add: &str, port: u16) -> bool {
    let target = format!("{}:{}", add, port);
    TcpStream::connect(target).is_ok()
}

pub fn open_port(addr: &str, port: u16) -> std::io::Result<TcpListener> {
    let target = format!("{}:{}", addr, port);
    let listener = TcpListener::bind(target)?;
    Ok(listener)
}

pub fn is_port_listening(port: u16) -> bool {
    let af_flags = AddressFamilyFlags::IPV4 | AddressFamilyFlags::IPV6;
    let protocol_flags = ProtocolFlags::TCP;
    let socket_info =
        get_sockets_info(af_flags, protocol_flags)
        .expect("error get_socket_info failed");

    for info in socket_info {
        if let ProtocolSocketInfo::Tcp(sinfo) = info.protocol_socket_info {
            if sinfo.local_port == port && sinfo.state == TcpState::Listen {
                return true;
            }
        }
    }
    false
}

pub async fn read_events_port_tokio(port: u16) -> Result<u32, (u32, Errno)> {
    let join_handle = task::spawn(async move {
        loop {
            if is_port_listening(port) {
                println!("The port {} is LISTENING in tokio", port);
                break;
            }
            tokio::time::sleep(Duration::from_millis(500)).await;
        }
    });

    match join_handle.await {
        Ok(_) => Ok(std::process::id()),
        Err(_) => Err((std::process::id(), Errno::UnknownErrno)),
    }
}

pub async fn read_events_port_scanner(port: u16) -> Result<(), ()> {
    task::spawn(async move {
        loop {
            if scan_port(port) {
                println!("The port {} is LISTENING in port scanner", port);
                break;
            }
            tokio::time::sleep(Duration::from_millis(500)).await;
        }
    });
    Ok(())
}