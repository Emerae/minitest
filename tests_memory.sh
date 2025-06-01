#!/bin/bash

# Tests mémoire pour minishell
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

VALGRIND_OPTS="--leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes"
SUPPRESSIONS="--suppressions=readline.supp"

echo "=== MEMORY TESTS FOR MINISHELL ==="

# Test 1: Commandes simples
echo -e "\n${YELLOW}Test 1: Simple commands${NC}"
echo -e "echo hello\nexit" | valgrind $VALGRIND_OPTS $SUPPRESSIONS ./minishell 2>valgrind_simple.log
if grep -q "All heap blocks were freed" valgrind_simple.log; then
    echo -e "${GREEN}✓ Simple commands: No leaks${NC}"
else
    echo -e "${RED}✗ Simple commands: Memory issues found${NC}"
fi

# Test 2: Pipes
echo -e "\n${YELLOW}Test 2: Pipes${NC}"
echo -e "echo hello | cat | cat\nexit" | valgrind $VALGRIND_OPTS $SUPPRESSIONS ./minishell 2>valgrind_pipes.log
if grep -q "All heap blocks were freed" valgrind_pipes.log; then
    echo -e "${GREEN}✓ Pipes: No leaks${NC}"
else
    echo -e "${RED}✗ Pipes: Memory issues found${NC}"
fi

# Test 3: Redirections
echo -e "\n${YELLOW}Test 3: Redirections${NC}"
echo -e "echo test > /tmp/test_out\ncat < /tmp/test_out\nexit" | valgrind $VALGRIND_OPTS $SUPPRESSIONS ./minishell 2>valgrind_redir.log
rm -f /tmp/test_out
if grep -q "All heap blocks were freed" valgrind_redir.log; then
    echo -e "${GREEN}✓ Redirections: No leaks${NC}"
else
    echo -e "${RED}✗ Redirections: Memory issues found${NC}"
fi

# Test 4: Variables d'environnement
echo -e "\n${YELLOW}Test 4: Environment variables${NC}"
echo -e "export TEST_VAR=hello\necho \$TEST_VAR\nunset TEST_VAR\nexit" | valgrind $VALGRIND_OPTS $SUPPRESSIONS ./minishell 2>valgrind_env.log
if grep -q "All heap blocks were freed" valgrind_env.log; then
    echo -e "${GREEN}✓ Environment: No leaks${NC}"
else
    echo -e "${RED}✗ Environment: Memory issues found${NC}"
fi

# Test 5: Heredocs
echo -e "\n${YELLOW}Test 5: Heredocs${NC}"
echo -e "cat << EOF\nline1\nline2\nEOF\nexit" | valgrind $VALGRIND_OPTS $SUPPRESSIONS ./minishell 2>valgrind_heredoc.log
if grep -q "All heap blocks were freed" valgrind_heredoc.log; then
    echo -e "${GREEN}✓ Heredocs: No leaks${NC}"
else
    echo -e "${RED}✗ Heredocs: Memory issues found${NC}"
fi

# Test 6: Builtins
echo -e "\n${YELLOW}Test 6: Builtins${NC}"
echo -e "pwd\ncd /tmp\npwd\ncd -\nexit" | valgrind $VALGRIND_OPTS $SUPPRESSIONS ./minishell 2>valgrind_builtins.log
if grep -q "All heap blocks were freed" valgrind_builtins.log; then
    echo -e "${GREEN}✓ Builtins: No leaks${NC}"
else
    echo -e "${RED}✗ Builtins: Memory issues found${NC}"
fi

# Test 7: Commandes complexes
echo -e "\n${YELLOW}Test 7: Complex commands${NC}"
echo -e "echo hello | grep h > /tmp/complex_test\ncat /tmp/complex_test\nexit" | valgrind $VALGRIND_OPTS $SUPPRESSIONS ./minishell 2>valgrind_complex.log
rm -f /tmp/complex_test
if grep -q "All heap blocks were freed" valgrind_complex.log; then
    echo -e "${GREEN}✓ Complex: No leaks${NC}"
else
    echo -e "${RED}✗ Complex: Memory issues found${NC}"
fi

# Test 8: Gestion d'erreur
echo -e "\n${YELLOW}Test 8: Error handling${NC}"
echo -e "nonexistent_command\necho test\nexit" | valgrind $VALGRIND_OPTS $SUPPRESSIONS ./minishell 2>valgrind_errors.log
if grep -q "All heap blocks were freed" valgrind_errors.log; then
    echo -e "${GREEN}✓ Error handling: No leaks${NC}"
else
    echo -e "${RED}✗ Error handling: Memory issues found${NC}"
fi

echo -e "\n${YELLOW}Summary:${NC}"
echo "Detailed logs saved as valgrind_*.log"
echo "Check individual files for specific leak information"

# Cleanup des logs si tout va bien
read -p "Delete log files? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    rm -f valgrind_*.log
    echo "Log files deleted"
fi