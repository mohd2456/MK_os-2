#!/bin/bash
# ============================================================
# MK OS - Docker Helper
# Wrapper for common Docker operations used by the C++ docker_manager.
# Commands: deploy-stack, backup-volumes, prune, health-check-all
# ============================================================

set -euo pipefail

# Configuration
BACKUP_DIR="${MK_DOCKER_BACKUP_DIR:-/opt/mk_os/backups/docker}"
COMPOSE_DIR="${MK_COMPOSE_DIR:-/opt/mk_os/stacks}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[DOCKER]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[DOCKER]${NC} $1"; }
log_error() { echo -e "${RED}[DOCKER]${NC} $1" >&2; }

# Check if Docker is available
check_docker() {
    if ! command -v docker &>/dev/null; then
        log_error "Docker is not installed"
        return 1
    fi
    if ! docker info &>/dev/null; then
        log_error "Docker daemon is not running or permission denied"
        return 1
    fi
    return 0
}

# Deploy a Docker Compose stack
deploy_stack() {
    local stack_name="${1:-}"
    local compose_file="${2:-}"

    if [ -z "$stack_name" ]; then
        log_error "Usage: deploy-stack <stack_name> [compose_file]"
        return 1
    fi

    if [ -z "$compose_file" ]; then
        compose_file="${COMPOSE_DIR}/${stack_name}/docker-compose.yml"
    fi

    if [ ! -f "$compose_file" ]; then
        log_error "Compose file not found: ${compose_file}"
        return 1
    fi

    log_info "Deploying stack: ${stack_name}"
    log_info "Compose file: ${compose_file}"

    docker compose -f "$compose_file" -p "$stack_name" up -d
    log_info "Stack ${stack_name} deployed successfully"
}

# Backup Docker volumes
backup_volumes() {
    local volume_filter="${1:-}"

    mkdir -p "$BACKUP_DIR"
    local timestamp
    timestamp=$(date +%Y%m%d_%H%M%S)

    local volumes
    if [ -n "$volume_filter" ]; then
        volumes=$(docker volume ls --format '{{.Name}}' | grep "$volume_filter" || echo "")
    else
        volumes=$(docker volume ls --format '{{.Name}}')
    fi

    if [ -z "$volumes" ]; then
        log_warn "No volumes found to backup"
        return 0
    fi

    local count=0
    while IFS= read -r vol; do
        if [ -z "$vol" ]; then continue; fi
        local backup_file="${BACKUP_DIR}/${vol}_${timestamp}.tar.gz"
        log_info "Backing up volume: ${vol}"

        docker run --rm \
            -v "${vol}:/source:ro" \
            -v "${BACKUP_DIR}:/backup" \
            alpine tar czf "/backup/${vol}_${timestamp}.tar.gz" -C /source . 2>/dev/null

        if [ -f "$backup_file" ]; then
            local size
            size=$(du -h "$backup_file" | cut -f1)
            log_info "  -> ${backup_file} (${size})"
            count=$((count + 1))
        fi
    done <<< "$volumes"

    log_info "Backed up ${count} volume(s) to ${BACKUP_DIR}"
}

# Prune unused Docker resources
prune() {
    local mode="${1:-safe}"

    case "$mode" in
        safe)
            log_info "Safe prune: removing dangling images and stopped containers"
            docker container prune -f
            docker image prune -f
            ;;
        full)
            log_info "Full prune: removing all unused resources"
            docker system prune -af --volumes
            ;;
        images)
            log_info "Pruning unused images"
            docker image prune -af
            ;;
        *)
            log_error "Unknown prune mode: ${mode}. Use: safe, full, images"
            return 1
            ;;
    esac

    log_info "Prune complete"
    docker system df
}

# Health check all running containers
health_check_all() {
    local json_output="["
    local first=true

    while IFS= read -r container_info; do
        if [ -z "$container_info" ]; then continue; fi

        local name status health image
        name=$(echo "$container_info" | cut -d'|' -f1)
        status=$(echo "$container_info" | cut -d'|' -f2)
        health=$(echo "$container_info" | cut -d'|' -f3)
        image=$(echo "$container_info" | cut -d'|' -f4)

        # Default health to "no-healthcheck" if empty
        if [ -z "$health" ] || [ "$health" = " " ]; then
            health="no-healthcheck"
        fi

        if [ "$first" = "true" ]; then
            first=false
        else
            json_output+=","
        fi

        json_output+="{\"name\":\"${name}\",\"status\":\"${status}\",\"health\":\"${health}\",\"image\":\"${image}\"}"
    done < <(docker ps --format '{{.Names}}|{{.Status}}|{{.Label "health"}}|{{.Image}}' 2>/dev/null || echo "")

    json_output+="]"
    echo "$json_output"
}

# Show usage
usage() {
    echo ""
    echo -e "${CYAN}MK OS Docker Helper${NC}"
    echo ""
    echo "Usage: $0 <command> [options]"
    echo ""
    echo "Commands:"
    echo "  deploy-stack <name> [compose_file]  - Deploy a Docker Compose stack"
    echo "  backup-volumes [filter]             - Backup Docker volumes"
    echo "  prune [safe|full|images]            - Prune unused resources"
    echo "  health-check-all                    - Check health of all containers"
    echo ""
    echo "Environment:"
    echo "  MK_DOCKER_BACKUP_DIR  - Backup directory (default: /opt/mk_os/backups/docker)"
    echo "  MK_COMPOSE_DIR        - Compose files directory (default: /opt/mk_os/stacks)"
    echo ""
}

# Main dispatch
main() {
    local command="${1:-help}"
    shift || true

    case "$command" in
        deploy-stack)
            check_docker || exit 1
            deploy_stack "$@"
            ;;
        backup-volumes)
            check_docker || exit 1
            backup_volumes "$@"
            ;;
        prune)
            check_docker || exit 1
            prune "$@"
            ;;
        health-check-all)
            check_docker || exit 1
            health_check_all
            ;;
        help|--help|-h)
            usage
            ;;
        *)
            log_error "Unknown command: ${command}"
            usage
            exit 1
            ;;
    esac
}

main "$@"
