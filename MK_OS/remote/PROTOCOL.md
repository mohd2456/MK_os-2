# MK OS - Remote PC Control Protocol

## Overview

MK (Mac side) communicates with the PC Agent over a persistent TCP connection using JSON Lines protocol (one JSON object per line, newline-terminated).

## Connection Details

| Parameter | Default | Env Var |
|-----------|---------|---------|
| Port | 9876 | `MK_PC_PORT` |
| PC IP | 192.168.1.100 | `MK_PC_IP` |
| Auth Token | (none) | `MK_PC_TOKEN` |

## Protocol

### Transport
- **Protocol**: TCP (persistent connection)
- **Format**: JSON Lines — one JSON object per line, terminated by `\n`
- **Encoding**: UTF-8
- **Max message size**: 1 MB

### Timeouts
- **Connect**: 5 seconds
- **Command execution**: 30 seconds
- **Idle connection**: 60 seconds (server-side)

### Request Format

```json
{"id": 1, "type": "shell", "payload": "dir /w", "token": "your_auth_token"}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | int | yes | Unique request ID (monotonically increasing) |
| `type` | string | yes | Command type (see below) |
| `payload` | string | yes | Primary argument for the command |
| `token` | string | yes | Authentication token |
| (extra fields) | varies | no | Command-specific extra fields |

### Response Format

```json
{"id": 1, "success": true, "stdout": "...", "stderr": "", "code": 0}
```

| Field | Type | Always | Description |
|-------|------|--------|-------------|
| `id` | int | yes | Echoed from request |
| `success` | bool | yes | Whether command succeeded |
| `error` | string | on failure | Error message |
| (data fields) | varies | on success | Command-specific response data |

## Command Types

### `ping`
Health check / connection verification.

**Request**: `{"id": 1, "type": "ping", "payload": "", "token": "xxx"}`
**Response**: `{"id": 1, "success": true, "message": "pong"}`

---

### `shell`
Execute a shell command on the PC.

**Request**: `{"id": 2, "type": "shell", "payload": "dir /w", "token": "xxx"}`
**Response**:
```json
{"id": 2, "success": true, "stdout": "...", "stderr": "", "code": 0}
```

---

### `read_file`
Read a file from the PC filesystem.

**Request**: `{"id": 3, "type": "read_file", "payload": "C:\\Users\\me\\file.txt", "token": "xxx"}`
**Response**:
```json
{"id": 3, "success": true, "content": "file contents here"}
```

---

### `write_file`
Write content to a file on the PC.

**Request**: `{"id": 4, "type": "write_file", "path": "/home/user/file.txt", "content": "hello", "token": "xxx"}`
**Response**:
```json
{"id": 4, "success": true, "message": "Written 5 bytes to /home/user/file.txt"}
```

---

### `open_app`
Launch an application on the PC.

**Request**: `{"id": 5, "type": "open_app", "payload": "chrome", "token": "xxx"}`
**Response**:
```json
{"id": 5, "success": true, "message": "Launched: chrome"}
```

---

### `system_info`
Get system information (OS, CPU, RAM, disk).

**Request**: `{"id": 6, "type": "system_info", "payload": "", "token": "xxx"}`
**Response**:
```json
{
  "id": 6, "success": true,
  "os": "Windows 10 (AMD64)",
  "cpu": "8 cores, 23% usage",
  "ram": "12GB / 16GB (75%)",
  "disk": "256GB / 512GB (50%)",
  "hostname": "MY-PC"
}
```

---

### `list_processes`
List running processes.

**Request**: `{"id": 7, "type": "list_processes", "payload": "", "token": "xxx"}`
**Response**:
```json
{"id": 7, "success": true, "output": "PID  NAME\n1234 chrome.exe\n..."}
```

---

### `kill_process`
Kill a process by name or PID.

**Request**: `{"id": 8, "type": "kill_process", "payload": "chrome.exe", "token": "xxx"}`
**Response**:
```json
{"id": 8, "success": true, "message": "Killed: chrome.exe"}
```

---

### `clipboard_get`
Read the PC clipboard contents.

**Request**: `{"id": 9, "type": "clipboard_get", "payload": "", "token": "xxx"}`
**Response**:
```json
{"id": 9, "success": true, "content": "clipboard text here"}
```

---

### `clipboard_set`
Write text to the PC clipboard.

**Request**: `{"id": 10, "type": "clipboard_set", "payload": "text to copy", "token": "xxx"}`
**Response**:
```json
{"id": 10, "success": true, "message": "Clipboard set"}
```

---

### `screenshot`
Take a screenshot of the PC screen.

**Request**: `{"id": 11, "type": "screenshot", "payload": "", "token": "xxx"}`
**Response**:
```json
{"id": 11, "success": true, "path": "/tmp/mk_screenshot_1234567890.png"}
```

---

### `list_files`
List files in a directory.

**Request**: `{"id": 12, "type": "list_files", "payload": "C:\\Users\\me\\Documents", "token": "xxx"}`
**Response**:
```json
{"id": 12, "success": true, "output": "[DIR]  Projects/\n[FILE] notes.txt (2KB)\n..."}
```

## Authentication

- Every request MUST include a `token` field
- Token is verified server-side on every request
- Invalid tokens receive: `{"success": false, "error": "Authentication failed"}`
- Connections from non-whitelisted IPs are immediately closed (when whitelist is configured)

## Error Handling

All errors follow this format:
```json
{"id": <request_id>, "success": false, "error": "<description>"}
```

Common errors:
- `"Authentication failed"` — wrong token
- `"Command timed out (30s)"` — command exceeded timeout
- `"Unknown command: xyz"` — unsupported command type
- `"No command provided"` — empty payload for commands that require one

## Security Considerations

1. **Token-based auth**: All requests require valid token
2. **IP whitelist**: Optional restriction to specific source IPs
3. **Audit logging**: All commands logged with timestamp and source IP
4. **Timeout enforcement**: No command runs longer than 30 seconds
5. **Local network only**: Not designed for internet exposure
6. **No encryption**: Traffic is plaintext — use on trusted networks only

## Future Considerations

- TLS encryption for the TCP connection
- File transfer (binary) via chunked encoding
- Streaming command output (for long-running processes)
- Multi-PC management (connect to multiple agents)
