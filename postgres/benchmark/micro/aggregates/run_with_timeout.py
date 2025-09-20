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

    except subprocess.TimeoutExpired:
        print(f"Timeout reached ({timeout_sec}s). Terminating process...")

        # Try to terminate gracefully first
        process.terminate()
        try:
            # Give it a moment to clean up
            stdout, stderr = process.communicate(timeout=2)
        except subprocess.TimeoutExpired:
            # Force kill if it doesn't respond to terminate
            print("Process didn't terminate gracefully. Force killing...")
            process.kill()
            stdout, stderr = process.communicate()

        # Re-raise the timeout exception
        raise subprocess.TimeoutExpired(cmd, timeout_sec, output=stdout, stderr=stderr)

    except Exception as e:
        # Clean up if any other exception occurs
        try:
            process.kill()
        except:
            pass
        raise e
