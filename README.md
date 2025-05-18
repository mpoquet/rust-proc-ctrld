# Control Daemon

## Launch project

### Rust : 

Go to rust rep

`cargo run --bin run_demon <port>` : command to launch demon on the specify port.  
`cargo run --bin run_client` : command to launch client prototype (not finished).

### C :

Requirements : 
- Flatbuffers >= 25.2.10
- Meson

Compilation : 

Clone repo

```bash
git clone https://github.com/mpoquet/rust-proc-ctrld.git
cd c
```

compile 

```bash
meson setup builddir
meson compile -C builddir
```

execute

```bash
cd builddir
./daemon 8080
```



### Tests :
- Flatbuffers
- pytest

First compile both projects then launch the tests

```bash
cd test_python
pytest test_daemon
pytest -s --disable-warnings test_overhead

```



## Description

This project aims to develop a lightweight and robust distributed application for executing and controlling the execution of processes across a set of machines. It will be implemented in both Rust and C. Comparisons between the languages will be made to best meet the requirements. We will refer to this application as a "Control Daemon". The application will use a daemon-based architecture, with a long-running process on each machine, providing a remote control interface.

## Credit

FORT Alexandre  
MAATI Mohamed-Yâ-Sîn  
FIEUX Telmo  
LAGIER Hadrien  

## License
- Code: Apache-2.0
- Everything else, in particular documentation and measurements: CC-BY-SA-4.0
