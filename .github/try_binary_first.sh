#!/bin/bash

MAX_RETRIES=3 # Safety cap to prevent infinite loops if the dependency tree is broken

try_binary_first() {
    # 1. --- Input Validation ---
    if [ "$#" -eq 0 ]; then
        echo "Usage: $0 <port> [+variants] ... [global_key=value ...]" >&2
        exit 1
    fi

    if ! command -v port &>/dev/null; then
        echo "Error: 'port' command not found." >&2
        exit 1
    fi

    # 2. --- Argument Parsing ---
    local global_options=()
    local job_queue=() 
    
    local current_port_name=""
    local current_port_variants=""

    _flush_port() {
        if [[ -n "$current_port_name" ]]; then
            local job_entry="${current_port_name}${current_port_variants}"
            job_queue+=("$job_entry")
        fi
        current_port_name=""
        current_port_variants=""
    }

    for arg in "$@"; do
        case "$arg" in
            *=*)
                # Global Key=Value
                global_options+=("$arg")
                ;;
            [-+]*)
                # Variant specifier (could be mixed like "+univ-debug")
                if [[ -z "$current_port_name" ]]; then
                    echo "Error: Variant '$arg' without port name." >&2
                    exit 1
                fi
                # Inject spaces before + or - to split combined variants (e.g., +a-b -> +a -b)
                # This ensures the 'port' command interprets them correctly as separate flags.
                
                current_port_variants+=" $arg"
                ;;
            *)
                # Port Name
                _flush_port
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

    # 3. --- Main Processing Loop ---
    echo "Processing ${#job_queue[@]} target(s) with global options: ${global_options[*]}"
    
    # We use a temp file to capture output because we need to both display it 
    # to the user (via tee) AND parse it for errors.
    local log_file
    log_file=$(mktemp)
    trap 'rm -f "$log_file"' EXIT

    for job in "${job_queue[@]}"; do
        read -r -a port_args <<< "$job"
        local target_name="${port_args[0]}"
        local attempt_counter=0
        local job_success=0

        echo "=========================================================="
        echo "TARGET: $job"
        echo "=========================================================="

        while [ $attempt_counter -lt $MAX_RETRIES ]; do
            ((attempt_counter++))

            echo
            echo "--- [Attempt $attempt_counter/$MAX_RETRIES] Binary install for: ${port_args[*]}"
            
            # Attempt Binary Install
            # -b: Binary only (fails if missing)
            # -N: Non-interactive (don't ask confirmation)
            local exit_code
            set -o pipefail
            sudo port -Nb install "${port_args[@]}" "${global_options[@]}" 2>&1 | tee "$log_file"
            exit_code=$?
            set +o pipefail
            echo "exit code: $exit_code"
            if [ $exit_code -eq 0 ]; then
                echo "--> SUCCESS: Installed ${target_name} via binary."
                job_success=1
                break
            else
            	echo "else clause"
                echo "--> FAILED: Binary install returned exit code $exit_code."
                
                # --- Diagnostic Logic ---
                # Search for the specific MacPorts error message identifying the missing binary
                # Regex looks for: "no binary archive found for <identifier>"
                local missing_port
                missing_port=$(grep -E "Failed to archivefetch " "$log_file" | sed -E 's/^.+Failed to archivefetch ([a-zA-Z0-9+-_.]+): version (@[a-zA-Z0-9._+-]+):.*/\1\2/')

                if [[ -n "$missing_port" ]]; then
                    # Trim whitespace
                    missing_port=$(echo "$missing_port" | xargs)
                    
                    echo "--> DIAGNOSIS: Missing binary archive for dependency: '$missing_port'"
                    echo "--> RECOVERY:  Compiling '$missing_port' from source..."
                    
                    # Attempt to install ONLY the missing dependency from source
                    # Note: We pass global options (like universal), but NOT the top-level variants
                    # because we are installing a specific dependency by name. 
                    # MacPorts automatically calculates required variants for dependencies 
                    # based on the tree, or defaults.
                    
                    if sudo port -Ns install "$missing_port" "${global_options[@]}"; then
                        echo "--> RECOVERY SUCCESS: '$missing_port' built and installed."
                        echo "--> ACTION: Retrying binary install for top-level '$target_name'..."
                        # The loop continues here, retrying the `port -b install <TopLevel>`
                        # which should now find the dependency satisfied.
                        continue 
                    else
                        echo "--> RECOVERY FAILED: Could not build '$missing_port' from source."
                        break # Stop trying this job
                    fi
                else
                    # Fallback for generic errors (checksum mismatch, configure error, etc)
                    # OR if we couldn't parse the specific dependency name.
                    echo "--> ERROR: Could not identify specific missing binary or encountered a generic error."
                    echo "--> Log output checks didn't match known patterns."
                    break 
                fi
            fi
        done

        if [ $job_success -eq 0 ]; then
            echo "xxx CRITICAL FAILURE: Unable to install '$target_name' after $attempt_counter attempts."
            echo "xxx Moving to next job (if any)..."
        fi

    done
    
    echo
    echo "All operations completed."
}
