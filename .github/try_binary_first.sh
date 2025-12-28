#!/bin/bash
try_binary_first() {
    # 1. --- Input Validation ---
    if [ "$#" -eq 0 ]; then
        echo "Usage: $0 <port> [+variants] [port2] ... [global_key=value ...]" >&2
        echo "Example: $0 boost +universal cairo -x11+quartz build_type=Release" >&2
        exit 1
    fi

    if ! command -v port &>/dev/null; then
        echo "Error: The 'port' command was not found." >&2
        exit 1
    fi

    # 2. --- Argument Parsing ---
    local global_options=()
    local job_queue=()       # Will hold strings like: "portname +var1 -var2"
    
    local current_port_name=""
    local current_port_variants=""

    # Function to commit the current port buffer to the job list
    _flush_port() {
        if [[ -n "$current_port_name" ]]; then
            # We combine the name and variants into a single string for storage
            # (Variants already have leading spaces added during collection)
            local job_entry="${current_port_name}${current_port_variants}"
            job_queue+=("$job_entry")
        fi
        current_port_name=""
        current_port_variants=""
    }

    # Loop through all arguments preserving order
    for arg in "$@"; do
        case "$arg" in
            *=*)
                # key=value pair -> GLOBAL option
                global_options+=("$arg")
                ;;
            [-+]*)
                # Starts with + or - -> VARIANT
                if [[ -z "$current_port_name" ]]; then
                    echo "Error: Variant '$arg' was specified before any port name." >&2
                    exit 1
                fi
                # Append to current variants for the active port
                current_port_variants+=" $arg"
                ;;
            *)
                # Likely a port name
                # 1. Flush the previous port if pending
                _flush_port
                # 2. Start tracking the new port
                current_port_name="$arg"
                ;;
        esac
    done
    # Flush the final remaining port
    _flush_port

    if [ "${#job_queue[@]}" -eq 0 ]; then
        echo "Error: No valid port names found to process." >&2
        exit 1
    fi

    # 3. --- Execution Loop ---
    echo "Starting processing for ${#job_queue[@]} target(s)."
    echo "Global options applied to all: ${global_options[*]}"
    echo "---------------------------------------------------"

    for job in "${job_queue[@]}"; do
        # 'job' contains "portname +var1 +var2".
        # We assume spaces separate components (standard for MacPorts names/variants).
        # We split the string into an array to pass safely to the port command.
        read -r -a port_args <<< "$job"
        local target_name="${port_args[0]}"

        echo
        echo "==> Processing: $job"
        
        # --- Attempt 1: Force Binary Installation ---
        # -b: fail if binary archive is not found (do not build from source)
        echo "    [Attempt 1] Trying binary installation..."
        
        if sudo port -b install "${port_args[@]}" "${global_options[@]}"; then
            echo "    [Success] Installed $target_name from binary archive."
        else
            # --- Attempt 2: Fallback to Source Build ---
            # Binary failed (doesn't exist, hash mismatch, or other error).
            echo "    [Fallback]  Binary installation unavailable or failed."
            echo "                Swapping to source build mode (-s)..."
            
            # -s: install by compiling from source (bypassing binaries)
            if sudo port -s install "${port_args[@]}" "${global_options[@]}"; then
                 echo "    [Success] Built and installed $target_name from source."
            else
                 echo "    [Error]   Failed to install $target_name (both binary and source attempts failed)."
                 # Optional: exit 1 here if you want to stop on the first failure. 
                 # Currently creates a best-effort behavior.
            fi
        fi
    done
    
    echo
    echo "All operations completed."
}