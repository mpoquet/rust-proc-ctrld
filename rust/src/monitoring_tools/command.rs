use nix::errno::Errno;
use tokio::process::Command;

use crate::proto::demon_generated::demon;



pub async fn exec_command(run_cmd: &demon::RunCommand<'_>) -> Result<u32, (u32, Errno)>{
    let path = run_cmd.path().expect("path not present");
    let args = run_cmd.args().unwrap_or_default();
    let envp = run_cmd.envp().unwrap_or_default();

    let mut command = Command::new(path);

    for i in 0..args.len() {
        command.arg(args.get(i));
    }

    for i in 0..envp.len() {
        let pair = envp.get(i);
        if let Some(eq_index) = pair.find('=') {
            let key = &pair[..eq_index];
            let value = &pair[eq_index + 1..];
            command.env(key, value);
        }
    }

    let output = command.output().await.map_err(|err| {
        let errno = Errno::from_raw(err.raw_os_error().unwrap_or(0));
        (std::process::id(), errno)
    })?;

    if output.status.success() {
        println!("Stdout: {}", String::from_utf8_lossy(&output.stdout));
        Ok(std::process::id())
    } else {
        let errno = Errno::from_raw(output.status.code().unwrap_or(1));
        Err((std::process::id(), errno))
    }
}