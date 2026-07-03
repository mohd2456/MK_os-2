#ifndef MK_DOCKER_MANAGER_CPP
#define MK_DOCKER_MANAGER_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <sstream>

// ===================================================================================
// MK DOCKER MANAGER
// Container orchestration via SSH to remote Docker daemons.
// Methods: listContainers, startContainer, stopContainer, deployStack,
// getContainerLogs, getContainerStats. Operates via SSH.
// ===================================================================================

struct MKContainer {
    std::string id;
    std::string name;
    std::string image;
    std::string status;       // running, stopped, exited, created
    std::string ports;
    float cpu_percent;
    long memory_mb;
    std::time_t created_at;
    std::string device;       // Which device this container is on
};

struct MKDockerStack {
    std::string name;
    std::string compose_file;
    std::string device;
    int container_count;
    std::string status;
};

struct MKDockerResult {
    bool success;
    std::string output;
    std::string error;
};

class MKDockerManager {
private:
    // Reference to SSH controller is managed externally
    // This class generates docker commands to be executed via SSH
    std::map<std::string, std::vector<MKContainer>> device_containers;
    std::vector<MKDockerStack> stacks;
    std::vector<std::string> operation_log;

    void logOperation(const std::string& op) {
        std::ostringstream entry;
        entry << "[" << std::time(nullptr) << "] " << op;
        operation_log.push_back(entry.str());
    }

public:
    MKDockerManager() {}

    // Generate command to list containers on a device
    std::string listContainersCmd() const {
        return "docker ps -a --format '{{.ID}}|{{.Names}}|{{.Image}}|{{.Status}}|{{.Ports}}'";
    }

    // Parse container list output
    std::vector<MKContainer> parseContainerList(const std::string& output,
                                                const std::string& device) {
        std::vector<MKContainer> containers;
        std::istringstream iss(output);
        std::string line;

        while (std::getline(iss, line)) {
            if (line.empty()) continue;
            MKContainer c;
            c.device = device;
            c.cpu_percent = 0.0f;
            c.memory_mb = 0;
            c.created_at = std::time(nullptr);

            // Parse pipe-delimited fields
            std::istringstream fields(line);
            std::getline(fields, c.id, '|');
            std::getline(fields, c.name, '|');
            std::getline(fields, c.image, '|');
            std::getline(fields, c.status, '|');
            std::getline(fields, c.ports, '|');

            if (!c.id.empty()) containers.push_back(c);
        }

        device_containers[device] = containers;
        return containers;
    }

    // Generate command to start a container
    std::string startContainerCmd(const std::string& name) const {
        return "docker start " + name;
    }

    // Generate command to stop a container
    std::string stopContainerCmd(const std::string& name) const {
        return "docker stop " + name;
    }

    // Generate command to restart a container
    std::string restartContainerCmd(const std::string& name) const {
        return "docker restart " + name;
    }

    // Generate command to deploy a docker-compose stack
    std::string deployStackCmd(const std::string& compose_file) const {
        return "docker-compose -f " + compose_file + " up -d";
    }

    // Generate command to get container logs
    std::string getContainerLogsCmd(const std::string& name, int tail_lines = 50) const {
        return "docker logs --tail " + std::to_string(tail_lines) + " " + name;
    }

    // Generate command to get container stats
    std::string getContainerStatsCmd(const std::string& name) const {
        return "docker stats --no-stream --format '{{.Name}}|{{.CPUPerc}}|{{.MemUsage}}' " + name;
    }

    // Parse container stats output
    void parseContainerStats(const std::string& output, const std::string& device) {
        std::istringstream iss(output);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.empty()) continue;
            std::istringstream fields(line);
            std::string name, cpu_str, mem_str;
            std::getline(fields, name, '|');
            std::getline(fields, cpu_str, '|');
            std::getline(fields, mem_str, '|');

            // Update container in our records
            auto it = device_containers.find(device);
            if (it != device_containers.end()) {
                for (auto& c : it->second) {
                    if (c.name == name) {
                        try {
                            // Remove % sign
                            if (!cpu_str.empty() && cpu_str.back() == '%')
                                cpu_str.pop_back();
                            c.cpu_percent = std::stof(cpu_str);
                        } catch (...) {}
                        break;
                    }
                }
            }
        }
    }

    // Generate command to pull an image
    std::string pullImageCmd(const std::string& image) const {
        return "docker pull " + image;
    }

    // Generate command to remove a container
    std::string removeContainerCmd(const std::string& name, bool force = false) const {
        return std::string("docker rm ") + (force ? "-f " : "") + name;
    }

    // Generate command to run a new container
    std::string runContainerCmd(const std::string& name, const std::string& image,
                               const std::vector<std::string>& ports = {},
                               const std::vector<std::string>& volumes = {},
                               const std::vector<std::string>& env_vars = {},
                               bool detach = true) const {
        std::ostringstream cmd;
        cmd << "docker run";
        if (detach) cmd << " -d";
        cmd << " --name " << name;
        for (const auto& p : ports) cmd << " -p " << p;
        for (const auto& v : volumes) cmd << " -v " << v;
        for (const auto& e : env_vars) cmd << " -e " << e;
        cmd << " " << image;
        return cmd.str();
    }

    // Record a successful operation
    void recordOperation(const std::string& device, const std::string& operation,
                        bool success) {
        logOperation(device + ": " + operation + " -> " + (success ? "OK" : "FAIL"));
    }

    // Get all known containers for a device
    std::vector<MKContainer> getContainers(const std::string& device) const {
        auto it = device_containers.find(device);
        if (it != device_containers.end()) return it->second;
        return {};
    }

    // Get running container count across all devices
    int runningCount() const {
        int count = 0;
        for (const auto& kv : device_containers) {
            for (const auto& c : kv.second) {
                if (c.status.find("Up") != std::string::npos ||
                    c.status.find("running") != std::string::npos) {
                    count++;
                }
            }
        }
        return count;
    }

    // Get total container count
    int totalCount() const {
        int count = 0;
        for (const auto& kv : device_containers) {
            count += static_cast<int>(kv.second.size());
        }
        return count;
    }

    // Get operation log
    std::vector<std::string> getLog() const { return operation_log; }

    // Get summary
    std::string getSummary() const {
        std::ostringstream ss;
        ss << "=== Docker Manager ===\n";
        ss << "  Tracked devices: " << device_containers.size() << "\n";
        ss << "  Total containers: " << totalCount() << "\n";
        ss << "  Running containers: " << runningCount() << "\n";
        ss << "  Stacks: " << stacks.size() << "\n";
        ss << "  Operations logged: " << operation_log.size() << "\n";
        return ss.str();
    }
};

#endif // MK_DOCKER_MANAGER_CPP
