use crossbeam_channel::{tick, select};

use rust_proc_ctrl::monitoring_tools::signal_tool::*;

fn main() -> Result<(), ctrlc::Error> {
    let ctrl_c_events = ctrl_channel()?;
    let ticks = tick(std::time::Duration::from_secs(1));

    loop {
        select! {
            recv(ticks) -> _ => {
                println!("Test de non bloquage!");
            }
            recv(ctrl_c_events) -> _ => {
                println!();
                println!("Got CTRL-C! Quit.");
                break;
            }
        }
    }

    Ok(())
}