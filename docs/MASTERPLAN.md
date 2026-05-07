# EronOS Masterplan (Real-World Track)

## Vision
Transform EronOS from educational kernel into a practical, internet-capable operating system with its own identity.

## Phase 1 — Core Reliability
- Stable memory allocator and regression tests.
- Process isolation hardening (separate address spaces).
- Panic diagnostics and crash dumps.
- Kernel/user ABI versioning.

## Phase 2 — Real Storage & Boot Ecosystem
- Initrd support.
- ext2 read/write filesystem driver.
- Unified `/etc` config model.
- Installer image + boot profiles.

## Phase 3 — Networking & Internet
- PCI enumeration.
- NIC drivers (e1000 first).
- IPv4 stack (ARP, IP, ICMP, UDP, TCP).
- DNS client + HTTP client.
- `ping`, `netstat`, `ifconfig` utilities.

## Phase 4 — Userland Completeness
- Minimal libc.
- Dynamic linker (optional after static toolchain milestone).
- Package manager and signed repositories.
- Multi-user permissions model.

## Phase 5 — Graphics Stack
- VBE framebuffer backend.
- Compositor + window server.
- Input stack unification (kbd/mouse events).
- Desktop shell.

## Phase 6 — Developer Experience
- CI with cross build + QEMU smoke tests.
- Kernel/user tests.
- SDK headers and examples.
- Reproducible releases.

## Product Principles
- Keep compatibility with existing shell culture.
- Prefer clear subsystem boundaries over monolithic code.
- Security and observability first.
