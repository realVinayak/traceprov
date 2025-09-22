import subprocess


def run_with_timeout(cmd, timeout_sec):
    try:
        # Start the process
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            # Important for process group handling
            start_new_session=True,
        )

        # Wait for the process to complete with timeout
        stdout, stderr = process.communicate(timeout=timeout_sec)
        return process.returncode, stdout, stderr

    except subprocess.TimeoutExpired as e:
        print(f"Timeout reached ({timeout_sec}s). Terminating process...")

        # Try to terminate gracefully first
        process.kill()
        raise e
