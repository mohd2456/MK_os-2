# Kernel

Low-level OS kernel components for MK OS bare-metal and VM deployments.

## Subdirectories

- **core/** - Kernel core initialization and main loop
- **driver/** - Hardware device drivers
- **fs/** - Filesystem abstraction layer
- **ipc/** - Inter-process communication
- **mm/** - Memory management (virtual memory, paging)
- **net/** - Kernel-level networking stack
- **power/** - Power management and sleep states
- **sched/** - Process/thread scheduler
- **security/** - Kernel-level security policies

## Entry Point

- **kernel_main.cpp** - Kernel boot and initialization sequence
