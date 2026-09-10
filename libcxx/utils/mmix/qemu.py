#!/usr/bin/env python3
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

"""Run an MMIX ELF test through the libc++ bare-metal executor interface."""

import argparse
import json
import math
import os
from pathlib import Path
import signal
import subprocess
import sys


def command_line(qemu, binary, arguments):
    # QemuOpts escapes a literal comma by doubling it, not by shell quoting.
    semihosting = "enable=on,target=native,chardev=semihost"
    for argument in (str(binary), *arguments):
        if "\0" in argument:
            raise ValueError("MMIX arguments cannot contain NUL")
        semihosting += ",arg=" + argument.replace(",", ",,")
    return [str(qemu), "-M", "virt,elf-startup-abi=argc-argv", "-cpu", "any",
            "-smp", "1", "-m", "256M", "-kernel", str(binary), "-display", "none",
            "-chardev", "stdio,id=semihost,signal=off", "-serial", "none",
            "-monitor", "none", "-no-reboot", "-semihosting-config", semihosting]


def run_process(command, directory, timeout):
    # A new process group also bounds descendants holding the output pipes.
    with subprocess.Popen(command, cwd=directory, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, start_new_session=True) as process:
        timed_out = False
        try:
            stdout, stderr = process.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            timed_out = True
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            stdout, stderr = process.communicate()
        return process.returncode, stdout, stderr, timed_out


def classify(returncode, stderr, timed_out):
    if timed_out:
        return "timeout", 124
    if returncode < 0:
        return "qemu-signal", 125
    # Both MMIX guest console handles use the semihosting stdout channel.
    # QEMU's stderr is therefore emulator diagnostics, not guest stderr.
    if stderr.strip():
        return "qemu-diagnostic", 125
    return "guest-exit", returncode


def validate_binary(binary):
    with binary.open("rb") as stream:
        header = stream.read(20)
    if (len(header) != 20 or header[:7] != b"\x7fELF\x02\x02\x01" or
            int.from_bytes(header[16:18], "big") != 2 or
            int.from_bytes(header[18:20], "big") != 80):
        raise ValueError("MMIX executor requires a native big-endian MMIX ELF executable")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--qemu", type=Path, required=True)
    parser.add_argument("--execdir", type=Path, required=True)
    parser.add_argument("--timeout", type=float, default=30)
    parser.add_argument("test_binary", type=Path)
    parser.add_argument("test_args", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)
    try:
        if not math.isfinite(args.timeout) or args.timeout <= 0:
            raise ValueError("MMIX executor timeout must be finite and positive")
        qemu, binary, directory = args.qemu.resolve(), args.test_binary.resolve(), args.execdir.resolve()
        if not qemu.is_file() or not os.access(qemu, os.X_OK) or not directory.is_dir():
            raise ValueError("MMIX executor requires an installed QEMU and existing execution directory")
        validate_binary(binary)
        result = run_process(command_line(qemu, binary, args.test_args), directory, args.timeout)
        returncode, stdout, stderr, timed_out = result
        kind, status = classify(returncode, stderr, timed_out)
        # Preserve transport bytes. Guest stdout/stderr share QEMU's console;
        # do not invent a separation that this semihosting transport lacks.
        sys.stdout.buffer.write(stdout)
        sys.stdout.buffer.flush()
        sys.stderr.buffer.write(stderr)
        sys.stderr.buffer.flush()
        with (directory / "mmix-executions.jsonl").open("a") as report:
            report.write(json.dumps({"kind": kind, "returncode": returncode,
                "executor_status": status, "timed_out": timed_out,
                "stdout": stdout.decode("utf-8", errors="replace"),
                "stderr": stderr.decode("utf-8", errors="replace")}) + "\n")
        if kind != "guest-exit":
            print(f"MMIX executor: {kind}", file=sys.stderr)
        return status
    except (OSError, ValueError) as error:
        print(f"MMIX executor: {error}", file=sys.stderr)
        return 125


if __name__ == "__main__":
    sys.exit(main())
