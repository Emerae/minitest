#!/bin/bash

# Test de la fonction de filtrage
run_minishell() {
    local cmd="$1"
    {
        echo "$cmd"
        echo "exit"
    } | timeout 5s ./minishell 2>/dev/null | \
    sed -n "/^minishell\$ $cmd$/{ n; p; q; }"
}

run_bash() {
    local cmd="$1"
    echo "$cmd" | bash 2>/dev/null | tail -n 1
}

echo "=== Test de filtrage ==="
cmd="echo hello"
echo "Command: $cmd"
echo "Minishell output: '$(run_minishell "$cmd")'"
echo "Bash output: '$(run_bash "$cmd")'"

echo
cmd="echo test + test"
echo "Command: $cmd"  
echo "Minishell output: '$(run_minishell "$cmd")'"
echo "Bash output: '$(run_bash "$cmd")'"