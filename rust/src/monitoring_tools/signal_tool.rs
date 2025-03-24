use nix::sys::signalfd::{SfdFlags, SignalFd};
use nix::sys::signal::{SigSet, Signal};

pub fn create_signalfd() -> std::io::Result<SignalFd> {
    let mut mask = SigSet::empty();
    mask.add(Signal::SIGINT);
    mask.add(Signal::SIGTERM);
    mask.add(Signal::SIGHUP);
    mask.thread_block().expect("error block signal");

    Ok(SignalFd::with_flags(&mask, SfdFlags::empty()).expect("error setting up flags to fd."))
}