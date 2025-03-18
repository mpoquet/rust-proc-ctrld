use std::{thread, time};

use rust_proc_ctrl::monitoring_tools::inotify_tool::*;
use rust_proc_ctrl::monitoring_tools::clone::*;


#[cfg(test)]
mod test {
    use super::*;

    mod test_clone {
        use super::*;

        #[test]
        fn test_handler() {
            setup_handler(handle_sigchld);
            execute_file(String::from("./foo"));

            thread::sleep(time::Duration::from_secs(5));
        }
    }

    mod tests_inotify {
        use super::*;

        #[test]
        fn test_inotify_blocking_events() {
            let mut inotify = create_inotify_watch_file("/home/wer/Documents/rust-proc-ctrld/rust/src/monitoring_tools");

            inotify_read_blocking(&mut inotify);

            inotify.close().expect("error closing inotify instance");
        }
    }
}