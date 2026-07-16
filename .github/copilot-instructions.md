# S3K Copilot Instructions

## Project Overview
S3K is a bare-metal RISC-V separation kernel:
- Target: `riscv64-unknown-elf`
- Privilege model: M-mode kernel with isolated S/U-mode processes
- Isolation: PMP-based, no MMU
- Runtime model: freestanding (no libc/runtime assumptions in kernel core)
- Determinism: avoid dynamic allocation and non-deterministic behavior in kernel paths

## Repository Structure
- `meson.build`: Top-level build entrypoint (`kern/`, `lib/`)
- `cross/`: Cross files (`rv64ima.ini`, `rv64imac.ini`)
- `kern/include/`: Kernel headers
- `kern/src/`: Kernel C and assembly sources
- `kern/platform/`: Platform-specific code and linker scripts
- `lib/include/`: Public S3K API headers for user programs
- `projects/hello/`: Single app example (with `qemu-run`, `gdb-run` targets)
- `projects/ipc/`: Multi-app IPC example (with `qemu-run`, `gdb-run` targets)
- `scripts/`: Utility scripts (including Docker wrapper)
- `API.md`: Kernel API details

## Coding Conventions

### General
- Keep changes minimal and focused; do not refactor unrelated code.
- Follow existing naming and file layout patterns in each directory.
- Prefer explicit, deterministic control flow in kernel code.
- Do not introduce heap allocation in kernel paths (`malloc`, `free`, equivalents).

### C (Kernel and Low-Level Code)
- Use fixed-width integer types (`uint8_t`, `uint32_t`, `uint64_t`, `uintptr_t`).
- Use `typedef`-based domain types already present in the codebase.
- Keep variable scope as small as possible.
- Use `static` for internal linkage helpers.
- Access MMIO through explicit `volatile` pointers/wrappers.
- Keep inline assembly small and localized; place complex sequences in `.S` files.

### RISC-V Assembly
- Respect RISC-V ABI conventions (`a*`, `t*`, `s*`, `sp`, `ra`).
- Use 64-bit operations where architectural state is 64-bit (`ld`, `sd`).
- Keep trap/context-switch logic in standalone assembly sources.

### Build Files
- In Meson files, list sources explicitly.
- Do not use wildcard globs for source discovery.

## Build and Run

### Prerequisites
- Toolchain: `riscv64-unknown-elf-*`
- Build tools: Meson, Ninja
- Optional runtime/debug tools: `qemu-system-riscv64`, `riscv64-unknown-elf-gdb`
- Optional environment wrapper: `./scripts/docker.sh`

### Build the kernel from repo root
```bash
meson setup builddir --cross-file cross/rv64imac.ini --buildtype=debugoptimized
ninja -C builddir
```

Alternative:
```bash
make build
```

### Clean builds
```bash
ninja -C builddir clean
# or
make clean
```

### Build and run example projects
Hello example:
```bash
cd projects/hello
meson setup builddir --cross-file ../../cross/rv64imac.ini
ninja -C builddir
ninja -C builddir qemu-run
```

IPC example:
```bash
cd projects/ipc
meson setup builddir --cross-file ../../cross/rv64imac.ini
ninja -C builddir
ninja -C builddir qemu-run
```

Debug target (when supported by platform setup):
```bash
ninja -C builddir gdb-run
```

## Change Checklist for Copilot
- Build files still use explicit source lists.
- No new kernel dynamic allocation.
- New low-level paths preserve deterministic behavior.
- Cross build still works with `cross/rv64imac.ini`.
- If applicable, validate with the relevant project `qemu-run` target.
