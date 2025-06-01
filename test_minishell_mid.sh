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
    grep -v "^minishell\\$" | \
    grep -v "^exit$" | \
    grep -v "^$" | \
    head -n 1
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

#echo
#echo "=== Environment Variables ==="
#test_command "echo \$HOME" "expand HOME variable"
#test_command "echo \$PATH" "expand PATH variable"

echo
echo "=== Exit Status Tests ==="
test_command "true" "true command"
test_command "false" "false command"

echo
echo "=== Advanced $? Tests ==="
echo -n "Testing: $? after successful command... "
result=$({
    echo "echo success"
    echo "echo \$?"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | tail -n 1)
if [ "$result" = "0" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: '0', Got: '$result'"
    ((FAILED++))
fi

echo -n "Testing: multiple $? usage... "
result=$({
    echo "false"
    echo "echo \$? \$? \$?"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | tail -n 1)
if [ "$result" = "1 1 1" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: '1 1 1', Got: '$result'"
    ((FAILED++))
fi

echo
echo "=== Quotes and Special Characters ==="
test_command "echo 'hello \"world\"'" "single quotes with double quotes inside"
test_command "echo \"hello 'world'\"" "double quotes with single quotes inside"
test_command "echo \"test\$HOME test\"" "double quotes with variable"
test_command "echo 'test\$HOME test'" "single quotes blocking variable"

echo
echo "=== Built-in Commands Extended ==="
test_no_crash "export NEW_VAR=test_value" "export new variable"
test_command "echo \$NEW_VAR" "use exported variable"
test_no_crash "unset NEW_VAR" "unset variable"
test_command "echo \$NEW_VAR" "check unset variable (should be empty)"

echo
echo "=== Edge Cases ==="
test_command "echo" "echo without arguments"
test_command "echo ''" "echo empty string"
test_command "echo \"\"" "echo empty double quotes"
test_no_crash "/bin/echo hello" "absolute path command"
test_no_crash "ls -l" "command with options"

echo
echo "=== Variable Expansion ==="
test_command "echo \$USER" "expand USER variable"
test_command "echo \$SHELL" "expand SHELL variable"
test_command "echo \$NONEXISTENT" "expand non-existent variable"

echo
echo "=== Complex Commands ==="
test_no_crash "echo hello; echo world" "multiple commands if supported"
test_no_crash "which ls" "which command"
test_no_crash "env | head -5" "env with pipe if supported"

echo
echo "=== Math and Expressions ==="
test_no_crash "expr 5 + 3" "basic addition"
test_no_crash "expr 10 - 4" "basic subtraction"
test_no_crash "expr 6 \\* 7" "basic multiplication"
test_no_crash "expr 15 / 3" "basic division"

echo -n "Testing: expr with variables... "
result=$({
    echo "export NUM=5"
    echo "expr \$NUM + 3"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | tail -n 1)
if [ "$result" = "8" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: '8', Got: '$result'"
    ((FAILED++))
fi

echo
echo "=== Redirection Tests (Basic) ==="
echo -n "Testing: output redirection... "
{
    echo "echo hello > /tmp/test_minishell_out"
    echo "exit"
} | ./minishell >/dev/null 2>&1
if [ -f "/tmp/test_minishell_out" ] && [ "$(cat /tmp/test_minishell_out)" = "hello" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
    rm -f /tmp/test_minishell_out
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
    rm -f /tmp/test_minishell_out
fi

echo -n "Testing: append redirection... "
{
    echo "echo first > /tmp/test_minishell_append"
    echo "echo second >> /tmp/test_minishell_append"
    echo "exit"
} | ./minishell >/dev/null 2>&1
if [ -f "/tmp/test_minishell_append" ] && [ "$(cat /tmp/test_minishell_append)" = "first
second" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
    rm -f /tmp/test_minishell_append
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
    rm -f /tmp/test_minishell_append
fi

echo -n "Testing: input redirection... "
echo "test input" > /tmp/test_minishell_input
result=$({
    echo "cat < /tmp/test_minishell_input"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | head -n 1)
if [ "$result" = "test input" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: 'test input', Got: '$result'"
    ((FAILED++))
fi
rm -f /tmp/test_minishell_input

echo
echo "=== Pipe Tests (Basic) ==="
echo -n "Testing: simple pipe... "
result=$({
    echo "echo hello | cat"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | head -n 1)
if [ "$result" = "hello" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: 'hello', Got: '$result'"
    ((FAILED++))
fi

echo -n "Testing: pipe with built-in... "
result=$({
    echo "echo world | cat"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | head -n 1)
if [ "$result" = "world" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: 'world', Got: '$result'"
    ((FAILED++))
fi

echo
echo "=== Heredoc Tests ==="
echo -n "Testing: basic heredoc... "
result=$({
    echo "cat << EOF"
    echo "line1"
    echo "line2"
    echo "EOF"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | head -n 2 | tr '\n' ' ' | sed 's/ $//')
if [ "$result" = "line1 line2" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: 'line1 line2', Got: '$result'"
    ((FAILED++))
fi
#test_should_fail "echo |" "incomplete pipe"
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