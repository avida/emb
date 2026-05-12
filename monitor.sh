#!/bin/bash

set -euo pipefail

DEFAULT_BAUD="57600"
DEFAULT_PORT="0"
PORT_ARG="${1:-$DEFAULT_PORT}"
BAUD_ARG="${2:-$DEFAULT_BAUD}"

usage() {
    echo "Usage: $0 [port-path|port-index] [baud-rate]"
    echo "Examples:"
    echo "  $0"
    echo "  $0 /dev/ttyUSB0"
    echo "  $0 0"
    echo "  $0 1 115200"
    exit 1
}

resolve_port() {
    local port_value="$1"
    local ports=()
    local resolved_port=""

    shopt -s nullglob
    ports=(/dev/ttyACM* /dev/ttyUSB*)
    shopt -u nullglob

    IFS=$'\n' ports=($(printf '%s\n' "${ports[@]}" | sort))
    unset IFS

    if [[ "$port_value" =~ ^[0-9]+$ ]]; then
        if [[ ${#ports[@]} -eq 0 ]]; then
            echo "No serial ports found under /dev/ttyACM* or /dev/ttyUSB*." >&2
            exit 1
        fi
        if (( port_value < 0 || port_value >= ${#ports[@]} )); then
            echo "Port index $port_value is out of range. Available ports:" >&2
            for idx in "${!ports[@]}"; do
                echo "  [$idx] ${ports[$idx]}" >&2
            done
            exit 1
        fi
        resolved_port="${ports[$port_value]}"
        echo "Using serial port index $port_value -> $resolved_port" >&2
    else
        resolved_port="$port_value"
    fi

    printf '%s\n' "$resolved_port"
}

SERIAL_PORT="$(resolve_port "$PORT_ARG")"

if [[ ! -e "$SERIAL_PORT" ]]; then
    echo "Serial port does not exist: $SERIAL_PORT" >&2
    exit 1
fi

echo "Listening on $SERIAL_PORT at $BAUD_ARG baud"
stty -F "$SERIAL_PORT" "$BAUD_ARG" raw -echo
exec cat "$SERIAL_PORT"