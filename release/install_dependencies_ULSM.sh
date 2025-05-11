#!/bin/bash

detect_distro() {
    if [ -f /etc/os-release ]; then
        source /etc/os-release
        case "$ID" in
            altlinux|alt) echo "altlinux" ;;
            manjaro)      echo "manjaro" ;;
            debian)       echo "debian" ;;
            ubuntu)       echo "ubuntu" ;;
            *)            echo "unknown" ;;
        esac
    elif [ -f /etc/altlinux-release ]; then
        echo "altlinux"
    elif [ -f /etc/debian_version ]; then
        echo "debian"
    else
        echo "unknown"
    fi
}

install_dependencies() {
    local distro="$1"
    echo "Installing dependencies for $distro..."

    case "$distro" in
        altlinux)
            sudo apt-get update
            sudo apt-get install -y lm-sensors dmidecode || { echo "Installation error!"; exit 1; }
            ;;
        manjaro)
            sudo pacman -Sy --noconfirm lm_sensors dmidecode || { echo "Installation error!"; exit 1; }
            ;;
        debian|ubuntu)
            sudo apt-get update
            sudo apt-get install -y lm-sensors dmidecode || { echo "Installation error!"; exit 1; }
            ;;
        *)
            echo "Unknown disribution. Please install dependencies yourself:"
            echo "lm-sensors (or lm_sensors), dmidecode"
            exit 1
            ;;
    esac

    echo "Dependencies are installed."
}

if ! command -v sensors &> /dev/null; then
    echo "lm-sensors not found. Installing..."
    distro=$(detect_distro)
    install_dependencies "$distro"
fi

if ! command -v dmidecode &> /dev/null; then
    echo "dmidecode not found. Installing..."
    distro=$(detect_distro)
    install_dependencies "$distro"
fi
