#ifndef MK_REMOTE_ACCESS_CPP
#define MK_REMOTE_ACCESS_CPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <fstream>
#include <sstream>
#include <functional>
#include <chrono>

// ===================================================================================
// MK REMOTE ACCESS — Connect to MK from your PC
// ===================================================================================
// MK runs 24/7 on the Mac server. You connect from your PC via:
//   - SSH (standard terminal access)
//   - Telegram (quick commands from phone)
//   - Local network socket (fast LAN connection from PC)
//   - Unix domain socket (for local apps on the Mac itself)
//
// This module handles:
//   - Session management (who's connected, since when)
//   - Command routing (remote commands → MK brain)
//   - Output streaming (MK responses → back to client)
//   - Authentication (simple token-based for local network)
//   - Connection health monitoring
//   - Rate limiting (prevent spam/DoS)
// ===================================================================================

enum class MKConnectionType {
    LOCAL_SOCKET,   // Unix domain socket (same machine)
    LAN_TCP,        // TCP over local network (PC → Mac)
    TELEGRAM,       // Telegram bot messages
    SSH             // Standard SSH terminal
};

enum class MKSessionState {
    CONNECTED,
    AUTHENTICATED,
    ACTIVE,
    IDLE,
    DISCONNECTED
};

struct MKRemoteSession {
    int sessionId;
    std::string clientName;     // "Mohammed's PC", "Phone", etc.
    std::string clientIP;
    MKConnectionType type;
    MKSessionState state;
    std::time_t connectedAt;
    std::time_t lastActivity;
    int commandsExecuted;
    std::string authToken;
    bool isAdmin;               // Full access?
};

struct MKRemoteCommand {
    int id;
    int sessionId;
    std::string raw;            // Raw command text
    std::string response;       // MK's response
    std::time_t receivedAt;
    double processingMs;
    bool success;
};

class MKRemoteAccess {
private:
    std::vector<MKRemoteSession> sessions;
    std::vector<MKRemoteCommand> commandLog;
    std::map<std::string, std::string> authTokens;  // token → client name
    int nextSessionId;
    int nextCommandId;
    int listenPort;
    std::string socketPath;
    bool accepting;
    
    // Rate limiting
    std::map<std::string, std::vector<std::time_t>> rateLimitMap;
    int maxCommandsPerMinute;
    
    // Stats
    int totalCommands;
    int totalSessions;
    long totalBytesReceived;
    long totalBytesSent;

    bool checkRateLimit(const std::string& clientIP) {
        std::time_t now = std::time(nullptr);
        auto& timestamps = rateLimitMap[clientIP];
        
        // Remove entries older than 60 seconds
        timestamps.erase(
            std::remove_if(timestamps.begin(), timestamps.end(),
                [now](std::time_t t) { return difftime(now, t) > 60.0; }),
            timestamps.end());
        
        if ((int)timestamps.size() >= maxCommandsPerMinute) {
            return false;  // Rate limited
        }
        timestamps.push_back(now);
        return true;
    }

public:
    MKRemoteAccess(int port = 9999, const std::string& sock = "/tmp/mk_os.sock")
        : nextSessionId(0), nextCommandId(0), listenPort(port), socketPath(sock),
          accepting(true), maxCommandsPerMinute(60), totalCommands(0),
          totalSessions(0), totalBytesReceived(0), totalBytesSent(0) {
        
        // Pre-register auth tokens
        authTokens["mk_admin_2026"] = "Mohammed";
        authTokens["mk_pc_token"] = "Mohammed's PC";
        
        std::cout << "[REMOTE] MK Remote Access server ready.\n";
        std::cout << "[REMOTE] TCP Port: " << listenPort << " | Socket: " << socketPath << "\n";
    }

    // Create a new session when client connects
    int connect(const std::string& clientIP, MKConnectionType type, 
                const std::string& token = "") {
        MKRemoteSession session;
        session.sessionId = nextSessionId++;
        session.clientIP = clientIP;
        session.type = type;
        session.state = MKSessionState::CONNECTED;
        session.connectedAt = std::time(nullptr);
        session.lastActivity = session.connectedAt;
        session.commandsExecuted = 0;
        session.isAdmin = false;
        
        // Authenticate
        if (!token.empty()) {
            auto it = authTokens.find(token);
            if (it != authTokens.end()) {
                session.clientName = it->second;
                session.authToken = token;
                session.state = MKSessionState::AUTHENTICATED;
                session.isAdmin = true;
                std::cout << "[REMOTE] Authenticated: " << session.clientName 
                          << " from " << clientIP << "\n";
            } else {
                std::cout << "[REMOTE] Auth FAILED for " << clientIP << "\n";
                session.state = MKSessionState::DISCONNECTED;
                return -1;
            }
        } else {
            session.clientName = "anonymous_" + std::to_string(session.sessionId);
            session.state = MKSessionState::ACTIVE;
        }
        
        sessions.push_back(session);
        totalSessions++;
        std::cout << "[REMOTE] Session #" << session.sessionId << " opened: " 
                  << session.clientName << "\n";
        return session.sessionId;
    }

    // Disconnect a session
    void disconnect(int sessionId) {
        for (auto& s : sessions) {
            if (s.sessionId == sessionId) {
                s.state = MKSessionState::DISCONNECTED;
                std::cout << "[REMOTE] Session #" << sessionId << " closed: " 
                          << s.clientName << "\n";
                break;
            }
        }
    }

    // Process a command from a remote session
    // Returns the response string to send back
    std::string processCommand(int sessionId, const std::string& command,
                                std::function<std::string(const std::string&)> handler) {
        // Find session
        MKRemoteSession* session = nullptr;
        for (auto& s : sessions) {
            if (s.sessionId == sessionId && s.state != MKSessionState::DISCONNECTED) {
                session = &s;
                break;
            }
        }
        if (!session) return "[ERROR] Invalid session.";
        
        // Rate limit check
        if (!checkRateLimit(session->clientIP)) {
            return "[ERROR] Rate limited. Slow down.";
        }
        
        // Log command
        MKRemoteCommand cmd;
        cmd.id = nextCommandId++;
        cmd.sessionId = sessionId;
        cmd.raw = command;
        cmd.receivedAt = std::time(nullptr);
        
        // Route to MK brain (the handler function)
        auto start = std::chrono::steady_clock::now();
        std::string response = handler(command);
        auto end = std::chrono::steady_clock::now();
        
        cmd.response = response;
        cmd.processingMs = std::chrono::duration<double, std::milli>(end - start).count();
        cmd.success = !response.empty();
        commandLog.push_back(cmd);
        
        // Update session
        session->lastActivity = std::time(nullptr);
        session->commandsExecuted++;
        session->state = MKSessionState::ACTIVE;
        totalCommands++;
        totalBytesReceived += command.size();
        totalBytesSent += response.size();
        
        return response;
    }

    // Built-in meta commands (before routing to MK brain)
    std::string handleMetaCommand(const std::string& command) {
        if (command == "/sessions") {
            std::stringstream ss;
            ss << "Active sessions:\n";
            for (const auto& s : sessions) {
                if (s.state != MKSessionState::DISCONNECTED) {
                    ss << "  #" << s.sessionId << " " << s.clientName 
                       << " [" << (int)s.type << "] cmds=" << s.commandsExecuted << "\n";
                }
            }
            return ss.str();
        }
        if (command == "/stats") {
            std::stringstream ss;
            ss << "Remote Access Stats:\n";
            ss << "  Total sessions: " << totalSessions << "\n";
            ss << "  Total commands: " << totalCommands << "\n";
            ss << "  Data received: " << (totalBytesReceived / 1024) << "KB\n";
            ss << "  Data sent: " << (totalBytesSent / 1024) << "KB\n";
            return ss.str();
        }
        if (command == "/ping") return "pong";
        if (command == "/uptime") {
            return "MK server is running. Sessions active: " + 
                   std::to_string(getActiveSessions());
        }
        return "";  // Not a meta command
    }

    int getActiveSessions() const {
        int count = 0;
        for (const auto& s : sessions) {
            if (s.state != MKSessionState::DISCONNECTED) count++;
        }
        return count;
    }

    // Cleanup stale sessions (no activity for 30 minutes)
    void cleanupStaleSessions() {
        std::time_t now = std::time(nullptr);
        for (auto& s : sessions) {
            if (s.state != MKSessionState::DISCONNECTED && 
                difftime(now, s.lastActivity) > 1800.0) {
                s.state = MKSessionState::IDLE;
            }
        }
    }

    void printStatus() const {
        std::cout << "\n━━━ [REMOTE ACCESS] ━━━\n";
        std::cout << "  Port: " << listenPort << " | Socket: " << socketPath << "\n";
        std::cout << "  Active Sessions: " << getActiveSessions() << "\n";
        std::cout << "  Total Commands: " << totalCommands << "\n";
        std::cout << "  Rate Limit: " << maxCommandsPerMinute << " cmd/min\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━\n";
    }
};

#endif // MK_REMOTE_ACCESS_CPP
