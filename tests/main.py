import subprocess
import time
import os
import signal
import sys

def start_demon():
    print("[INFO] Lancement du démon...")
    process = subprocess.Popen(
        ["cargo", "run", "--bin", "run_demon"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        preexec_fn=os.setsid
    )
    time.sleep(1)  # Donne un peu de temps au démon pour démarrer
    return process

def stop_demon(process):
    print("[INFO] Arrêt du démon...")
    os.killpg(os.getpgid(process.pid), signal.SIGTERM)
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        print("[WARN] Le démon ne s'est pas arrêté proprement.")

def run_pytest():
    print("[INFO] Lancement des tests pytest...")
    result = subprocess.run(
        ["pytest", "test_demon.py", "-v"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        encoding="utf-8"
    )
    print(result.stdout)
    if result.returncode != 0:
        print(result.stderr)
    return result.returncode

def main():
    demon = start_demon()
    try:
        code = run_pytest()
    finally:
        stop_demon(demon)
    sys.exit(code)

if __name__ == "__main__":
    main()