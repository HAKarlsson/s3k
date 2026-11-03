# S3K - Safe and Secure Separation Kernel

[![CI Build](https://github.com/HAKarlsson/s3k/actions/workflows/build.yml/badge.svg)](https://github.com/HAKarlsson/s3k/actions/workflows/build.yml)

S3K is an experimental bare-metal capability-based separation kernel for RISC-V, built for safety- and security-critical systems such as avionics.
It supports dynamic partitioning with strict spatial and temporal isolation. The kernel is implemented in C and assembly with a small trusted computing base (TCB) and minimal hardware requirements (RISC-V PMP, machine mode and user mode).

Trusted user processes can dynamically create and destroy partitions. Time is a first-class capability for time-driven cyclic schedules. Memory capabilities configure the RISC-V PMP for dynamic spatial isolation. Monitor capabilities let trusted processes supervise and reconfigure others (boot, health checks, recovery). IPC capabilities move data, capabilities, and execution time donations between partitions.

Using the temporal fence instruction (see [PULP's CVA6 implementation](https://github.com/pulp-platform/cva6)), S3K provides time protection between partitions, reducing covert channels from core-local microarchitectural state. To mitigate off-core channels, pair S3K with hardware or platform measures such as cache partitioning, scratchpad/SRAM usage, or flushing shared resources on partition switches.

On multicore RV64 configurations, secondary harts acquire the boot-ready flag after hart 0 publishes initialized kernel state with release ordering. A FIFO ticket lock serializes handlers, scheduler decisions, and process-state transitions. Once a hart has taken a ticket it waits for its turn, even if its timer expires. Revocation returns partial progress when the timer expires or another hart is queued.

The scheduler performs one lookup at the current time-slot index, without tracking or traversing earlier frames. Only frame starts contain runnable entries: missing a frame's start means waiting for a later frame start. Empty entries and unavailable processes advance the local timer to the next elementary slot boundary. A voluntary yield releases the process and waits for the already-programmed timer deadline.

Trap handlers return a signed `proc_result_t`: zero waits after releasing the outgoing process, a positive PCB address resumes without reloading PMP, and the negative PCB address requests a PMP reload. Supported PCB addresses are nonzero and fit in the positive range of `intptr_t`. IPC and monitor operations complete their own handoffs: they save the syscall result before releasing the caller and return the final process result. Their syscall handlers only marshal arguments; the dispatcher passes results through unchanged. Only scheduling, process handoffs, and explicit `s3k_sync()` calls request a reload. The post-handler tests the sign and negates a reload result to recover the PCB address, then performs PMP programming and context restoration outside the lock.

PMP revocation is deliberately lazy: changing saved PMP state, locally or remotely, does not itself reload the hardware. Other harts may disable a running process's PMP entries, but cannot change their addresses or enable entries while it is acquired. Disabled entries retain their saved addresses so an unlocked reload can safely observe older permissions. Call `s3k_sync()` to apply the current process's saved PMP explicitly; this does not voluntarily yield or change its timeout.

Capability grants and IPC share capability-transfer dispatch while retaining separate destination authorization and channel permission checks. Synchronous IPC shares request, response, and delivery mechanics without changing blocking or donation semantics. PMP syscall handlers call the memory operations directly, with monitor authorization in the monitor handlers.

## Repository Organization

- `cross/` : Cross-compilation configuration files.
- `kern/` : Kernel source code and header files.
- `lib/` : Kernel API library (libs3k).
- `platform/` : Platform-specific kernel configurations and linker scripts.
- `projects/` : Example projects demonstrating S3K capabilities.
- `scripts/` : Build and development scripts (e.g., Docker wrapper).
- `API.md` : Kernel API documentation.
- `LICENSE` : License file.
- `meson.build` : Meson build configuration.
- `meson.options` : Kernel build options.
- `README.md` : This file.

## Compilation instructions

Requirements: Docker (rootless).

### Compiling only the kernel

```bash
./scripts/docker.sh
meson setup builddir --cross-file=cross/rv64imac.ini
ninja -C builddir
```

The multicore regression checks cover ticket ordering, locked process-state transitions, direct schedule lookup, partial revocation, lazy PMP updates, and trap return values. They require `riscv64-linux-gnu-gcc` with its libc development headers and `qemu-riscv64`:

```bash
sh tests/run-multicore.sh
```

## Compilation instructions for hello project

```bash
./scripts/docker.sh
cd projects/hello
meson setup builddir --cross-file=../../cross/rv64imac.ini
ninja -C builddir
ninja -C builddir qemu-run
```

## Future Work

S3K is a research prototype under active development. We have many ideas for improvements but cannot guarantee timelines.

- **Formal verification** [Ongoing] 
  - Formalizing core kernel abstractions and proving security/safety properties using HOL4.
  - Extending verification to cover the kernel implementation (binary level).
- **Kernel reimplementation** [Ongoing]
  - Reimplementing the kernel in a formally-verifiable language designed for safety-critical systems.
- **Extended platform support**
  - Add 32-bit RISC-V support and additional boards/SoCs.
- **Fine-grained monitor capabilities**
  - Restrict monitor privileges for more granular partition management.
- **Asynchronous IPC**
  - Support message queues alongside current synchronous IPC and message buffers.
- **Interrupt capabilities**
  - Let partitions handle hardware interrupts with maintained isolation and security.
- **Performance testing and benchmarking**
  - Establish benchmark suites and comprehensive test coverage.
- **Documentation and examples**
  - Expand API documentation and add example projects.
- **OS partition support**
  - Run real-time operating systems and Linux as isolated partitions within S3K.
- **User-space library**
  - Provide a higher-level abstraction over the kernel API for application developers.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgements

This work was supported by KTH CDIS (Centre for Cyber Defence and Information Security) and the Intel Corporation.

## Related Papers

H. Karlsson, and R. Guanciale, "Partitioning Kernel with Capability Controlled Temporal and Spatial Partitioning," in 2025 IEEE 46th Real-Time Systems Symposium (RTSS'25), Dec. 2025.

H. A. Karlsson, “Minimal partitioning kernel with time protection
and predictability,” in Proc. IEEE European Symposium on Security
and Privacy Workshops (EuroS&PW’24). IEEE, 2024, pp. 234–241.

K. Palmskog, et al. "Trustworthy Verification of RISC-V Binaries Using Symbolic Execution in HolBA." arXiv preprint arXiv:2503.14135 (2025).
