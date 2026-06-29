# System

Core system infrastructure for MK OS. Handles boot, entry point, daemon management, and shell interaction.

## Key Components

- **mk_entry.cpp** - Unified entry point. Includes all modules, defines MKSystem struct, runs the REPL loop
- **boot.cpp** - System boot sequence and initialization
- **daemon.cpp** - Background daemon for persistent operation
- **shell.cpp** - Interactive shell and command parsing
- **service_manager.cpp** - Manages system services lifecycle
- **diagnostics.cpp** - Health checks, memory usage, uptime stats
- **auto_updater.cpp** - GitHub auto-update mechanism
- **remote_access.cpp** - Remote PC control interface
- **task_scheduler.cpp** - Scheduled tasks, reminders, and cron-like operations
- **logger.cpp** - Structured logging system
