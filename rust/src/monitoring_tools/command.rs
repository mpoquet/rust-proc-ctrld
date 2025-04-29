use crate::proto::demon_generated::demon;



pub fn exec_command(run_cmd: &demon::RunCommand<'_>) -> tokio::process::Command{
    let path = run_cmd.path().expect("path not present");
    let args = run_cmd.args().unwrap_or_default();
    let envp = run_cmd.envp().unwrap_or_default();

    let mut command = tokio::process::Command::new(path);
    for arg in args {
        command.arg(arg);
    }
    for pair in envp {
        if let Some(eq_index) = pair.find('=') {
            let key = &pair[..eq_index];
            let value = &pair[eq_index + 1..];
            command.env(key, value);
        }
    }

    command
}