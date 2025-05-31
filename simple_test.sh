#!/bin/bash

echo "=== Test simple pour déboguer ==="

echo "Test 1: echo simple"
{
    echo "echo hello"
    echo "exit"
} | ./minishell

echo -e "\n---\n"

echo "Test 2: echo avec +"
{
    echo "echo test + test"
    echo "exit"
} | ./minishell

echo -e "\n---\n"

echo "Test 3: expr simple"
{
    echo "expr 1 + 1"
    echo "exit"
} | ./minishell

echo -e "\n---\n"

echo "Test 4: exit status"
{
    echo "true"
    echo "echo \$?"
    echo "exit"
} | ./minishell