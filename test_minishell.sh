#!/bin/bash

# Couleurs pour l'affichage
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Compteurs
PASSED=0
FAILED=0

# Fonction pour exécuter une commande dans minishell et récupérer la sortie
run_minishell() {
    local cmd="$1"
    {
        echo "$cmd"
        echo "exit"
    } | timeout 5s ./minishell 2>/dev/null | \
    grep -v "minishell\$" | \
    grep -v "create_node:" | \
    grep -v "subtsi check" | \
    grep -v "find_delimiter:" | \
    grep -v "append_cmd3:" | \
    grep -v "current_input->input" | \
    grep -v "^de$" | \
    grep -v "^i = " | \
    grep -v "^ii = " | \
    grep -v "^exit$" | \
    grep -v "^$" | \
    tail -n 1
}

# Fonction pour exécuter une commande dans bash
run_bash() {
    local cmd="$1"
    echo "$cmd" | bash 2>/dev/null | tail -n 1
}

# Fonction de test avec comparaison
test_command() {
    local cmd="$1"
    local description="$2"
    
    echo -n "Testing: $description... "
    
    local minishell_output=$(run_minishell "$cmd")
    local bash_output=$(run_bash "$cmd")
    
    if [ "$minishell_output" = "$bash_output" ]; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC}"
        echo "  Command: $cmd"
        echo "  Expected: '$bash_output'"
        echo "  Got:      '$minishell_output'"
        ((FAILED++))
    fi
}

# Fonction de test simple (juste vérifier qu'il n'y a pas de crash)
test_no_crash() {
    local cmd="$1"
    local description="$2"
    
    echo -n "Testing: $description... "
    
    local output=$({
        echo "$cmd"
        echo "exit"
    } | timeout 5s ./minishell 2>&1)
    local exit_code=$?
    
    if [ $exit_code -eq 0 ] || [ $exit_code -eq 1 ]; then
        echo -e "${GREEN}NO CRASH${NC}"
        ((PASSED++))
    else
        echo -e "${RED}CRASH/TIMEOUT${NC}"
        echo "  Command: $cmd"
        echo "  Exit code: $exit_code"
        ((FAILED++))
    fi
}

# Fonction de test pour vérifier que les erreurs sont bien détectées
test_should_fail() {
    local cmd="$1"
    local description="$2"
    
    echo -n "Testing: $description... "
    
    local output=$({
        echo "$cmd"
        echo "exit"
    } | timeout 3s ./minishell 2>&1 | grep -i "syntax error\|command not found")
    
    if [ -n "$output" ]; then
        echo -e "${GREEN}CORRECTLY FAILED${NC}"
        ((PASSED++))
    else
        echo -e "${RED}SHOULD HAVE FAILED${NC}"
        echo "  Command: $cmd"
        ((FAILED++))
    fi
}

echo "=== MINISHELL TEST SUITE ==="
echo

# Vérifier que minishell existe
if [ ! -f "./minishell" ]; then
    echo -e "${RED}Error: ./minishell not found${NC}"
    exit 1
fi

echo "=== Basic Commands ==="
test_command "echo hello" "echo simple"
test_command "echo hello world" "echo multiple words"
test_command "echo 'hello world'" "echo with quotes"
test_command "echo \"hello world\"" "echo with double quotes"

echo
echo "=== Commands with + character ==="
test_command "echo test + test" "echo with +"
test_command "echo a+b+c" "echo with multiple +"
test_command "echo 'math: 2+2'" "echo with + in quotes"

echo
echo "=== Built-ins ==="
test_command "pwd" "pwd command"
test_no_crash "cd /tmp" "cd absolute path"
test_no_crash "cd" "cd without args"

echo
echo "=== Environment Variables ==="
test_command "echo \$HOME" "expand HOME variable"
test_command "echo \$PATH" "expand PATH variable"

echo
echo "=== Exit Status Tests ==="
test_command "true" "true command"
test_command "false" "false command"

echo
echo "=== Expression Tests (the main goal) ==="
test_no_crash "expr 1 + 1" "basic expr test"
test_no_crash "expr \$? + \$?" "expr with \$? + \$?"

echo
echo "=== Error Cases (should still be detected) ==="
test_should_fail "echo |" "incomplete pipe"
test_should_fail "echo <" "incomplete redirect"
test_should_fail "echo >" "incomplete output redirect"
test_should_fail "echo test & test" "ampersand (should fail)"
test_should_fail "echo test * test" "asterisk (should fail)"

echo
echo "=== No Crash Tests ==="
test_no_crash "ls" "ls command"
test_no_crash "echo \$PATH" "expand PATH"
test_no_crash "env" "env command"

echo
echo "=== RESULTS ==="
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
echo -e "Total:  $((PASSED + FAILED))"

if [ $FAILED -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed! ✓${NC}"
    exit 0
else
    echo -e "\n${RED}Some tests failed! ✗${NC}"
    exit 1
fi