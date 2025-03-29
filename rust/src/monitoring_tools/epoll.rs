use nix::libc::{self, epoll_create1, epoll_ctl, epoll_event, epoll_wait, fcntl, EPOLLIN, EPOLL_CTL_ADD, F_SETFL, O_NONBLOCK};

pub fn set_non_blocking(fd: i32) -> Result<(), ()>{
    let flags = unsafe {
        libc::fcntl(fd, libc::F_GETFL)
    };

    if flags < 0 {
        return Err(());
    }
    if unsafe {fcntl(fd, F_SETFL, flags | O_NONBLOCK)} < 0 {
        return Err(());
    }

    Ok(())
}

pub fn add_fd_to_epoll(epoll_fd: i32, watch_fd: i32) -> Result<(), ()> {
    let mut event = epoll_event {
        events: (EPOLLIN) as u32,
        u64: watch_fd as u64,
    };

    let result = unsafe {
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, watch_fd, &mut event)
    };
    if result < 0 {
        return Err(());
    }
    Ok(())
}

pub fn create_epoll() -> Result<i32, ()> {
    let epoll_fd = unsafe {epoll_create1(0)};
    if epoll_fd < 0 {
        return Err(());
    }
    Ok(epoll_fd)
}

pub fn epoll_event_empty() -> epoll_event {
    epoll_event { events: 0, u64: 0 }
}