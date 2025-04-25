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

    tokio::spawn(async move {
        match command.output().await {
            Ok(_) => todo!(),
            Err(err) => {
                let errno = Errno::from_raw(err.raw_os_error().unwrap_or(0));
                return Err::<u32, (u32, Errno)>((std::process::id(), errno));
            },
        }
    });
    Ok(std::process::id())
}