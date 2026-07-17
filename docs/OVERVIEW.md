# S3K Overview

S3K is a capability-based partitioning kernel for RISC-V systems. It exposes a minimal set of abstractions covering process management, inter-process communication (IPC), memory management, and time slicing. Capabilities enforce strict access control and isolation between processes, enabling fine-grained, secure partitioning of system resources.

## Hardware Requirements

S3K requires the following minimum hardware:

- A 64-bit RISC-V CPU with `M` (machine) and `U` (user) privilege levels.
- Physical Memory Protection (PMP) for enforcing memory access permissions.
- A timer interrupt source for time slicing and preemption.

To mitigate intra-core side-channel attacks, S3K additionally requires:

- Temporal fence (`fence.t`) support to prevent intra-core timing attacks.
- Scratchpad memory (or equivalent) for hosting the kernel and sensitive application data.

## Design Principles

S3K is built around the following principles:

- **Capability-based security:** All resources — memory, time slices, IPC channels — are accessed through capabilities: unforgeable tokens that grant specific rights.
- **Minimalism:** The kernel exposes only the abstractions essential for secure partitioning, avoiding unnecessary complexity.
- **Capability-based partitioning:** A partition is defined by the capabilities held by its processes, not by explicit configuration.
- **Information flow control:** Every system call prevents information leakage between processes. Kernel control flow depends solely on data accessible to the calling process, so sensitive information cannot be inferred from observed system behavior.
- **Deterministic execution:** Given the same inputs and state, the kernel always produces the same outputs.
- **Time-deterministic scheduling:** Given a fixed schedule, a ready process is always dispatched at the same time, on the same hart, and with the same time-slice length.
- **Fault isolation:** Faults in one partition do not affect the execution of other partitions.
- **Policy freedom:** The kernel enforces no specific security policy; users define their own through the capability system.
- **Non-preemptible system calls:** System calls run atomically to completion. Each is designed to execute in a bounded, small number of instructions.
- **No dynamic allocation:** All kernel data structures are statically allocated at build time, giving predictable memory usage and eliminating fragmentation and leaks.
- **No kernel threads:** All kernel operations execute in the context of the calling process or the scheduler, simplifying the design and reducing the attack surface.
- **Capability-driven major-frame scheduling:** Time is organized into a fixed-length major frame, subdivided into minor frames defined by time-slice capabilities. Each capability specifies the hart, start slot, length, assigned process, and enabled state of its minor frame.
  - Each hart has an independent major frame and scheduler.
  - The kernel performs no dynamic scheduling decisions; all scheduling is fully determined by the time-slice capabilities held by processes, which users configure at runtime.
  - The kernel strictly enforces the start time and length of each minor frame, ensuring predictable, repeatable execution.
  - A process may hold time-slice capabilities on multiple harts, but can execute on at most one hart at a time. If a ready process is scheduled on multiple harts simultaneously, it is dispatched on the hart with the lowest ID.
