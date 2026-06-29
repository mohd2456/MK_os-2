# Plugins

Extensible plugin system for external integrations and add-on features.

## Key Components

- **plugin_interface.cpp** - Base MKPlugin class with virtual lifecycle methods (init, handle, shutdown)
- **manager.cpp** - Plugin registry: register, enable, disable, list plugins
- **telegram.cpp** - Telegram bot integration (24/7 messaging from phone)
- **pc_helper.cpp** - Remote PC control and system automation
- **mesh_network.cpp** - Distributed mesh networking between MK instances
- **web_monitor.cpp** - Web endpoint monitoring and alerts
- **thermal_monitor.cpp** - Hardware temperature monitoring
- **self_correction.cpp** - Auto-correction of plugin errors
- **custom.cpp** - User-defined custom plugin template
