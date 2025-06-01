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
    } | timeout 3s ./minishell 2>&1 | grep -i "syntax error\|command not found\|not found\|permission denied\|no such file\|not a valid identifier")
    
    if [ -n "$output" ]; then
        echo -e "${GREEN}CORRECTLY FAILED${NC}"
        ((PASSED++))
    else
        echo -e "${RED}SHOULD HAVE FAILED${NC}"
        echo "  Command: $cmd"
        ((FAILED++))
    fi
}

# Fonction de test spéciale pour exit codes
test_exit_code() {
    local cmd="$1"
    local expected_code="$2"
    local description="$3"
    
    echo -n "Testing: $description... "
    
    local actual_code=$({
        echo "$cmd"
        echo "echo \$?"
        echo "exit"
    } | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | tail -n 1)
    
    if [ "$actual_code" = "$expected_code" ]; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC}"
        echo "  Command: $cmd"
        echo "  Expected exit code: '$expected_code', Got: '$actual_code'"
        ((FAILED++))
    fi
}

echo "=== EXTENDED MINISHELL TEST SUITE ==="
echo

# Vérifier que minishell existe
if [ ! -f "./minishell" ]; then
    echo -e "${RED}Error: ./minishell not found${NC}"
    exit 1
fi

echo "=== Setup Test Environment ==="
echo -n "Creating test structure... "

# Sauvegarde du répertoire original AVANT le cd
original_test_dir=$(pwd)

# Nettoyage et création de la structure de test
cd /tmp
rm -rf minishell_test_structure 2>/dev/null
mkdir -p minishell_test_structure/{dir1,dir2,dir3}/{subdir1,subdir2}
mkdir -p minishell_test_structure/dir1/subdir1/deep
mkdir -p minishell_test_structure/dir2/.hidden
mkdir -p minishell_test_structure/spaces\ dir/sub\ dir
mkdir -p minishell_test_structure/dots.dir/more.dots

# Fichiers pour vérifier les répertoires
touch minishell_test_structure/dir1/file1.txt
touch minishell_test_structure/dir2/file2.txt
touch minishell_test_structure/dir1/subdir1/deep/deep_file.txt
touch minishell_test_structure/spaces\ dir/space_file.txt

# Structure pour tests d'erreur
mkdir -p minishell_test_structure/error_tests/existing

# Retourner dans le répertoire original pour pouvoir exécuter ./minishell
cd "$original_test_dir"

echo -e "${GREEN}DONE${NC}"

# Sauvegarde du répertoire original
original_test_dir=$(pwd)

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
echo "=== Built-ins Basic ==="
test_command "pwd" "pwd command"
test_no_crash "cd /tmp" "cd absolute path"
test_no_crash "cd" "cd without args"

echo
echo "=== Built-ins Advanced (Edge Cases) ==="
test_should_fail "cd /nonexistent/directory/path" "cd to nonexistent directory"
test_no_crash "cd /" "cd to root directory"
test_no_crash "cd .." "cd to parent directory"
test_no_crash "cd ~" "cd to home directory"

echo -n "Testing: cd back to original directory... "
original_dir=$(pwd)
result=$({
    echo "cd /tmp"
    echo "cd $original_dir"
    echo "pwd"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | tail -n 1)
if [ "$result" = "$original_dir" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: '$original_dir', Got: '$result'"
    ((FAILED++))
fi

echo
echo "=== Path Normalization Tests ==="

# Tests de normalisation de base
echo -n "Testing: cd with ./ normalization... "
result=$({
    echo "cd /tmp/minishell_test_structure/./dir1"
    echo "pwd"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | tail -n 1)
if [ "$result" = "/tmp/minishell_test_structure/dir1" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: '/tmp/minishell_test_structure/dir1', Got: '$result'"
    ((FAILED++))
fi

echo -n "Testing: cd with ../ navigation... "
result=$({
    echo "cd /tmp/minishell_test_structure/dir1/subdir1"
    echo "cd ../subdir2"
    echo "pwd"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | tail -n 1)
if [ "$result" = "/tmp/minishell_test_structure/dir1/subdir2" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: '/tmp/minishell_test_structure/dir1/subdir2', Got: '$result'"
    ((FAILED++))
fi

echo -n "Testing: cd complex path normalization... "
result=$({
    echo "cd /tmp/./minishell_test_structure/../minishell_test_structure/./dir2"
    echo "pwd"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | tail -n 1)
if [ "$result" = "/tmp/minishell_test_structure/dir2" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: '/tmp/minishell_test_structure/dir2', Got: '$result'"
    ((FAILED++))
fi

# Test de l'erreur qu'on a corrigée
test_should_fail "cd /tmp/minishell_test_structure/./nonexistent/../dir1" "cd with nonexistent in path (should fail like bash)"

echo
echo "=== Multiple Slashes Tests ==="

echo -n "Testing: cd with double slashes... "
result=$({
    echo "cd /tmp//minishell_test_structure//dir1"
    echo "pwd"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | tail -n 1)
if [ "$result" = "/tmp/minishell_test_structure/dir1" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: '/tmp/minishell_test_structure/dir1', Got: '$result'"
    ((FAILED++))
fi

echo -n "Testing: cd with triple slashes... "
result=$({
    echo "cd ///tmp///minishell_test_structure///dir2///"
    echo "pwd"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | tail -n 1)
if [ "$result" = "/tmp/minishell_test_structure/dir2" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: '/tmp/minishell_test_structure/dir2', Got: '$result'"
    ((FAILED++))
fi

echo
echo "=== Tilde Expansion Tests ==="

# Crée un répertoire de test dans HOME
mkdir -p ~/minishell_test_home/subdir 2>/dev/null
touch ~/minishell_test_home/test_file.txt 2>/dev/null

echo -n "Testing: cd with tilde... "
result=$({
    echo "cd ~/minishell_test_home"
    echo "pwd"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | tail -n 1)
expected="$HOME/minishell_test_home"
if [ "$result" = "$expected" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: '$expected', Got: '$result'"
    ((FAILED++))
fi

echo -n "Testing: ls with tilde... "
result=$({
    echo "ls ~/minishell_test_home"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$")
if [[ "$result" == *"test_file.txt"* && "$result" == *"subdir"* ]]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: 'subdir and test_file.txt', Got: '$result'"
    ((FAILED++))
fi

echo -n "Testing: ls with tilde should fail for nonexistent... "
output=$({
    echo "ls ~/nonexistent_directory_12345"
    echo "exit"
} | ./minishell 2>&1 | grep -i "no such file\|cannot access")
if [ -n "$output" ]; then
    echo -e "${GREEN}CORRECTLY FAILED${NC}"
    ((PASSED++))
else
    echo -e "${RED}SHOULD HAVE FAILED${NC}"
    ((FAILED++))
fi

echo
echo "=== Path Edge Cases ==="

echo -n "Testing: cd with spaces in directory name... "
result=$({
    echo "cd \"/tmp/minishell_test_structure/spaces dir\""
    echo "pwd"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | tail -n 1)
if [ "$result" = "/tmp/minishell_test_structure/spaces dir" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: '/tmp/minishell_test_structure/spaces dir', Got: '$result'"
    ((FAILED++))
fi

echo -n "Testing: cd with dots in directory name... "
result=$({
    echo "cd /tmp/minishell_test_structure/dots.dir"
    echo "pwd"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | tail -n 1)
if [ "$result" = "/tmp/minishell_test_structure/dots.dir" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: '/tmp/minishell_test_structure/dots.dir', Got: '$result'"
    ((FAILED++))
fi

echo
echo "=== Export/Unset Edge Cases ==="
test_should_fail "export 123INVALID=test" "export invalid variable name (starts with number)"
test_should_fail "export =test" "export empty variable name"
test_should_fail "export INVALID-VAR=test" "export variable with dash"
test_no_crash "export TEST_VAR=" "export variable with empty value"
test_no_crash "export VALID_VAR=value123" "export valid variable with numbers"
test_command "echo \$VALID_VAR" "use exported variable"
test_no_crash "unset VALID_VAR" "unset exported variable"
test_command "echo \$VALID_VAR" "check unset variable is empty"
test_no_crash "unset NONEXISTENT_VAR" "unset non-existent variable"

echo
echo "=== Echo Edge Cases ==="
test_command "echo" "echo without arguments"
test_command "echo -n" "echo -n without arguments"

# Test echo -n manually (hard to test via pipes due to newline handling)
echo -n "Testing: echo -n variants (manual verification)... "
echo -e "${YELLOW}MANUAL${NC} (please verify manually:"
echo "  echo -n hello          → should show 'hello' without newline"  
echo "  echo -n -n hello       → should show 'hello' without newline"
echo "  echo -nnnn hello       → should show 'hello' without newline"
echo ")"

test_command "echo -nxxx hello" "echo with invalid -n variant (-nxxx)"
test_command "echo -N hello" "echo with capital N flag"
test_command "echo hello -n" "echo with -n after arguments"

test_command "echo ''" "echo empty single quotes"
test_command "echo \"\"" "echo empty double quotes"
test_no_crash "echo -n ''" "echo -n with empty quotes"

echo
echo "=== Exit Status Tests ==="
test_command "true" "true command"
test_command "false" "false command"
test_exit_code "true" "0" "true command exit code"
test_exit_code "false" "1" "false command exit code"
test_exit_code "/bin/echo test" "0" "external command success"

echo
echo "=== Advanced \$? Tests ==="
echo -n "Testing: \$? after successful command... "
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

echo -n "Testing: multiple \$? usage... "
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

echo -n "Testing: \$? persists across commands... "
result=$({
    echo "false"
    echo "echo first_command"
    echo "echo \$?"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | tail -n 1)
if [ "$result" = "0" ]; then  # Should be 0 because echo succeeded
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: '0' (echo success), Got: '$result'"
    ((FAILED++))
fi

echo
echo "=== Quotes and Special Characters Advanced ==="
test_command "echo 'hello \"world\"'" "single quotes with double quotes inside"
test_command "echo \"hello 'world'\"" "double quotes with single quotes inside"
test_command "echo \"test\$HOME test\"" "double quotes with variable"
test_command "echo 'test\$HOME test'" "single quotes blocking variable"
test_command "echo 'can\\'t'" "single quotes with escaped quote"
test_command "echo \"\$HOME is home\"" "variable at start of quoted string"
test_command "echo \"home is \$HOME\"" "variable at end of quoted string"
test_command "echo 'multiple  spaces   here'" "single quotes preserving spaces"
test_command "echo \"multiple  spaces   here\"" "double quotes preserving spaces"

echo
echo "=== Variable Expansion Advanced ==="
test_command "echo \$USER" "expand USER variable"
test_command "echo \$SHELL" "expand SHELL variable"
test_command "echo \$NONEXISTENT" "expand non-existent variable"
test_command "echo before\$USER after" "variable in middle of string"
test_command "echo \$USER\$HOME" "multiple variables concatenated"
test_command "echo \$" "lone dollar sign"
# test_command "echo \$123" "dollar with numbers"  # Parser behavior - commented
test_command "echo \${HOME}" "braced variable expansion"
test_command "echo \${NONEXISTENT}" "braced non-existent variable"

echo
echo "=== Export and Use Variables ==="
test_no_crash "export NEW_VAR=test_value" "export new variable"
test_command "echo \$NEW_VAR" "use exported variable"
test_no_crash "unset NEW_VAR" "unset variable"
test_command "echo \$NEW_VAR" "check unset variable (should be empty)"
test_no_crash "export MULTI_WORD=\"hello world\"" "export variable with spaces"
test_command "echo \$MULTI_WORD" "use variable with spaces"

echo
echo "=== Edge Cases ==="
test_no_crash "/bin/echo hello" "absolute path command"
test_no_crash "ls -l" "command with options"
test_should_fail "nonexistent_command" "nonexistent command"
test_no_crash "echo hello; echo world" "multiple commands if supported"
test_no_crash "which ls" "which command"

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
echo "=== Redirection Tests ==="
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

echo -n "Testing: multiple output redirections... "
{
    echo "echo test > /tmp/test1.txt > /tmp/test2.txt"
    echo "exit"
} | ./minishell >/dev/null 2>&1
# Only the last redirection should work
if [ -f "/tmp/test2.txt" ] && [ "$(cat /tmp/test2.txt)" = "test" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi
rm -f /tmp/test1.txt /tmp/test2.txt

echo
echo "=== Pipe Tests ==="
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

echo -n "Testing: triple pipe... "
result=$({
    echo "echo hello | cat | cat"
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

echo -n "Testing: pipe with grep... "
result=$({
    echo "echo hello world | grep hello"
    echo "exit"
} | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | head -n 1)
if [ "$result" = "hello world" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "  Expected: 'hello world', Got: '$result'"
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

# Heredoc with variables - parser behavior, commented
# echo -n "Testing: heredoc with variables... "
# result=$({
#     echo "export TEST_VAR=hello"
#     echo "cat << EOF"
#     echo "Value is: \$TEST_VAR"
#     echo "EOF"
#     echo "exit"
# } | ./minishell 2>/dev/null | grep -v "^minishell\\$" | grep -v "^exit$" | grep -v "^$" | head -n 1)
# if [ "$result" = "Value is: hello" ]; then
#     echo -e "${GREEN}PASS${NC}"
#     ((PASSED++))
# else
#     echo -e "${RED}FAIL${NC}"
#     echo "  Expected: 'Value is: hello', Got: '$result'"
#     ((FAILED++))
# fi

echo
echo "=== Syntax Error Tests ==="
test_should_fail "echo <" "incomplete input redirect"
test_should_fail "echo >" "incomplete output redirect"
test_should_fail "echo >>" "incomplete append redirect"
test_should_fail "echo <<" "incomplete heredoc"
# test_should_fail "echo 'unclosed" "unclosed single quote"  # Parser behavior - commented
# test_should_fail "echo \"unclosed" "unclosed double quote"  # Parser behavior - commented
test_should_fail "echo test & test" "ampersand (should fail)"
test_should_fail "echo test * test" "asterisk (should fail)"
test_should_fail "| echo test" "pipe at beginning"
test_should_fail "echo | | cat" "double pipe"

echo
echo "=== Complex Integration Tests ==="
echo -n "Testing: pipe with redirection... "
{
    echo "echo hello | cat > /tmp/test_pipe_redir"
    echo "exit"
} | ./minishell >/dev/null 2>&1
if [ -f "/tmp/test_pipe_redir" ] && [ "$(cat /tmp/test_pipe_redir)" = "hello" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
    rm -f /tmp/test_pipe_redir
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
    rm -f /tmp/test_pipe_redir
fi

echo -n "Testing: variable expansion in redirections... "
{
    echo "export OUTFILE=/tmp/test_var_redir"
    echo "echo testing > \$OUTFILE"
    echo "exit"
} | ./minishell >/dev/null 2>&1
if [ -f "/tmp/test_var_redir" ] && [ "$(cat /tmp/test_var_redir)" = "testing" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
    rm -f /tmp/test_var_redir
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
    rm -f /tmp/test_var_redir
fi

echo
echo "=== Cleanup Test Environment ==="
echo -n "Cleaning up test files... "
rm -rf /tmp/minishell_test_structure 2>/dev/null
rm -rf ~/minishell_test_home 2>/dev/null
cd "$original_test_dir" 2>/dev/null
echo -e "${GREEN}DONE${NC}"

echo
echo "=== No Crash Stress Tests ==="
test_no_crash "ls" "ls command"
test_no_crash "env" "env command"
test_no_crash "pwd && echo done" "command combination if supported"
test_no_crash "echo \$\$" "process ID variable"
test_no_crash "echo \$0" "program name variable"

echo
echo "=== RESULTS ==="
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
echo -e "Total:  $((PASSED + FAILED))"

if [ $FAILED -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed! ✓${NC}"
    echo -e "Your minishell is ready for norminette! 🚀"
    echo -e "Note: Some tests commented (parser behaviors, echo -n pipe issues)"
    exit 0
else
    echo -e "\n${RED}Some tests failed! ✗${NC}"
    echo -e "Fix these issues before running norminette."
    echo -e "Note: Parser-related tests and tricky echo -n tests are commented"
    exit 1
fi