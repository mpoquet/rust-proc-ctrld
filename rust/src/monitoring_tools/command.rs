use crate::proto::demon_generated::demon;



pub fn exec_command(run_cmd: &demon::RunCommand<'_>) -> tokio::process::Command {
    let path = run_cmd.path().expect("path not present");
    let mut args = run_cmd.args().unwrap_or_default().iter().collect::<Vec<_>>();
    let envp = run_cmd.envp().unwrap_or_default();

    // Si args[0] est identique au path absolu ou son nom de base, on l’enlève
    if let Some(first_arg) = args.first() {
        let path_basename = std::path::Path::new(path)
            .file_name()
            .and_then(|s| s.to_str())
            .unwrap_or("");
        if *first_arg == path || *first_arg == path_basename {
            args.remove(0);
        }
    }

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
