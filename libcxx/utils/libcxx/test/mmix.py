# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

"""Installed static MMIX libc++ testing, with no host execution probes."""

import json
import math
import os
from pathlib import Path
import shlex
import sys

TRIPLE = "mmix-unknown-unknown"


def validate_profile(macros, provider, optimization):
    if provider not in ("llvm-libc", "newlib") or optimization not in ("O0", "O2"):
        raise ValueError("MMIX requires a supported C provider and O0 or O2")
    expected = {"__cplusplus": "201703L", "_LIBCPP_ABI_VERSION": "1",
                "_LIBCPP_ABI_NAMESPACE": "__1",
                "_LIBCPP_LIBC_LLVM_LIBC": str(int(provider == "llvm-libc")),
                "_LIBCPP_LIBC_NEWLIB": str(int(provider == "newlib"))}
    for name in ("THREADS", "FILESYSTEM", "LOCALIZATION", "RANDOM_DEVICE",
                 "MONOTONIC_CLOCK", "TIME_ZONE_DATABASE", "UNICODE", "WIDE_CHARACTERS"):
        expected["_LIBCPP_HAS_" + name] = "0"
    if any(macros.get(name) != value for name, value in expected.items()):
        raise ValueError("MMIX installed libc++ profile or provider mismatch")
    if not {"__mmix__", "__cpp_exceptions", "__cpp_rtti"} <= macros.keys() or "NDEBUG" in macros:
        raise ValueError("MMIX requires the target compiler, exceptions, RTTI and assertions")
    return {"mmix", "target=" + TRIPLE, "c++17", "stdlib=libc++", "stdlib=llvm-libc++",
            "optimization=" + ("none" if optimization == "O0" else "speed"),
            "std-at-least-c++03", "std-at-least-c++11", "std-at-least-c++14", "std-at-least-c++17"}


def configure(config, lit_config):
    from libcxx.test.dsl import Feature, compilerMacros, hasCompileFlag, sourceBuilds
    from libcxx.test.features import compiler, libcxx_macros, carveouts
    from libcxx.test.format import CxxStandardLibraryTest

    # Do not allow generic lit parameters to silently change this tested profile
    # or replace its execution guard with a host command.
    if set(lit_config.params) - {"config_map"}:
        lit_config.fatal("MMIX configuration does not accept generic lit parameter overrides")
    source = Path(config.mmix_source).resolve()
    resource = Path(config.mmix_resource).resolve()
    sysroot = Path(config.mmix_sysroot).resolve()
    # Preserve clang++'s executable spelling, which selects the C++ Driver.
    cxx = Path(config.mmix_compiler).absolute()
    output = Path(config.mmix_output).resolve()
    provider, opt = config.mmix_provider, config.mmix_optimization
    qemu = getattr(config, "mmix_qemu", None)
    timeout = getattr(config, "mmix_timeout", 30)
    library = config.mmix_library
    if library not in ("libcxx", "libcxxabi"):
        lit_config.fatal("MMIX supports the libcxx and libcxxabi test roots")
    if any(output == p or output.is_relative_to(p) for p in (source, resource, sysroot, cxx.parent)):
        lit_config.fatal("MMIX test output overlaps an input installation or source tree")
    required = [cxx, cxx.parent / "ld.lld", resource / f"include/{TRIPLE}/c++/v1/__config_site"]
    required += [resource / f"lib/{TRIPLE}" / name for name in
                 ("libc++.a", "libc++abi.a", "libunwind.a", "clang_rt.crtbegin.o", "clang_rt.crtend.o")]
    if not sysroot.is_dir() or not all(p.is_file() for p in required) or not os.access(cxx, os.X_OK):
        lit_config.fatal("MMIX requires a complete installed compiler, sysroot and C++ resource")
    if qemu and (not Path(qemu).is_file() or not os.access(qemu, os.X_OK)):
        lit_config.fatal("MMIX requires the selected installed QEMU executor")
    if not math.isfinite(timeout) or timeout <= 0:
        lit_config.fatal("MMIX requires a finite positive execution timeout")
    output.mkdir(parents=True, exist_ok=True)
    config.name = f"MMIX-{library}-{provider}-{opt}"
    config.test_source_root = str(source / library / "test")
    config.test_exec_root = str(output)
    config.test_format = CxxStandardLibraryTest()
    config.recursiveExpansionLimit = 10
    config.environment = {"PATH": str(cxx.parent) + os.pathsep + "/usr/bin:/bin",
                          "LC_ALL": "C", "TMPDIR": str(output)}
    config.substitutions = [
        ("%{cxx}", shlex.quote(str(cxx))),
        ("%{flags}", shlex.join([f"--target={TRIPLE}", f"--cstdlib={provider}",
            f"--sysroot={sysroot}", f"-resource-dir={resource}"])),
        ("%{compile_flags}", shlex.join(["-std=c++17", "-" + opt, "-fexceptions", "-frtti", "-UNDEBUG",
            "-I", str(source / "libcxx/test/support")])),
        ("%{link_flags}", ""),
        ("%{benchmark_flags}", ""),
        ("%{exec}", shlex.join([sys.executable, "-c",
            "import sys; sys.exit('MMIX execution is not configured; no guest was run')"])),
        ("%{python}", shlex.quote(sys.executable)), ("%{cxx_std}", "cxx17"),
        ("%{libcxx-dir}", shlex.quote(str(source / "libcxx"))),
        ("%{include-dir}", shlex.quote(str(resource / f"include/{TRIPLE}/c++/v1"))),
        ("%{target-include-dir}", shlex.quote(str(resource / f"include/{TRIPLE}/c++/v1"))),
        ("%{lib-dir}", shlex.quote(str(resource / f"lib/{TRIPLE}"))),
    ]
    if qemu:
        executor = shlex.join([sys.executable, "-B", str(source / "libcxx/utils/mmix/qemu.py"),
                              "--qemu", str(Path(qemu).resolve()), "--timeout", str(timeout)])
        config.substitutions = [(name, executor + " --execdir %{temp} --" if name == "%{exec}" else value)
                                for name, value in config.substitutions]
    macros = compilerMacros(config)
    try:
        config.available_features = validate_profile(macros, provider, opt)
    except ValueError as error:
        lit_config.fatal(str(error))
    # These upstream feature groups use preprocessing only. Do not evaluate the
    # default feature set's programOutput, locale, host OS or debugger probes.
    features = compiler.features + libcxx_macros.features + carveouts.features
    features += [Feature(name="verify-support",
                         when=lambda cfg: hasCompileFlag(cfg, "-Xclang -verify-ignore-unexpected"))]
    for feature in features:
        for action in feature.getActions(config):
            action.applyTo(config)
    if not sourceBuilds(config, "#include <string>\nint main() { std::string s(\"mmix\"); return s.size() != 4; }"):
        lit_config.fatal("MMIX installed libc++ compile/link probe failed")
    (output / "mmix-configuration.json").write_text(json.dumps({
        "features": sorted(config.available_features), "substitutions": config.substitutions,
        "provider": provider, "optimization": opt, "execution": "qemu" if qemu else "disabled",
        "macros": {name: value for name, value in macros.items()
                   if name.startswith(("_LIBCPP_", "__cpp_")) or name in ("__mmix__", "__cplusplus")},
    }, indent=2) + "\n")
    lit_config.note(f"{config.name}: installed configuration; guest execution {'uses QEMU' if qemu else 'disabled'}")
