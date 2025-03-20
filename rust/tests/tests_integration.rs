use std::{thread, time};
use std::time::Duration;
use std::process::Command;

use rust_proc_ctrl::monitoring_tools::inotify_tool::*;
use rust_proc_ctrl::monitoring_tools::clone::*;
use rust_proc_ctrl::monitoring_tools::socket::*;


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
        fn test_inotify_blocking_events_create_dir() {
            let path = "src/monitoring_tools";
            let dir = format!("{}/tmp_dir", path);
            let mut inotify = create_inotify_watch_file(path);

            thread::sleep(Duration::from_millis(100));

            Command::new("mkdir")
                .args([&dir])
                .output()
                .expect("error test : create dir");

            inotify_read_blocking(&mut inotify);

            Command::new("rm")
                .args([String::from("-rf"), dir])
                .output()
                .expect("error test : remove dir");

            inotify.close().expect("error closing inotify instance");
        }
    }

    mod test_socket {
        use super::*;

        #[test]
        fn test_ck_is_socket_open() {
            let res = is_socket_open("127.0.0.1:80800000");

            assert!(!res)
        }

        #[test]
        fn test_ck_is_socket_open_v2() {
            let addr = "127.0.0.1:8080";
            let listener = open_socket(addr)
                .expect("error test : socket failed open.");
            let res = is_socket_open(addr);
            drop(listener);

            assert!(res)
        }
    }
}