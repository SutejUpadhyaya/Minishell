# Minishell

Minishell is a Unix-style command-line shell implemented in C.  
The project focuses on process management, command parsing, directory navigation, and interaction with the `/proc` virtual filesystem.

The shell is implemented from scratch without external shell libraries, using low-level system calls for process creation, execution, and signal handling.

---

## Overview

Minishell accepts user input, tokenizes commands, executes supported built-ins internally, and runs external programs using `fork()` and `execvp()`.

The shell maintains a prompt that reflects the current working directory and handles keyboard interrupts without terminating the session.

The repository includes:

- the full source implementation (`minishell.c`)
- a precompiled executable (`minishell`, macOS build)

Recompiling locally is recommended for portability.

---

## Core Capabilities

### Built-In Commands

| Command | Description |
|--------|-------------|
| `cd <path>` | Changes the working directory. Supports `~`, absolute, and relative paths. |
| `pwd` | Prints the current working directory. |
| `lf` | Lists visible files in the current directory (excluding `.` and `..`). |
| `lp` | Enumerates running system processes via `/proc`. |
| `exit` | Cleanly terminates the shell. |

If a command is not one of the built-ins, Minishell treats it as an external executable.

---

## External Program Execution

Minishell runs external programs using the standard Unix process model:

- `fork()` — creates a child process  
- `execvp()` — replaces the child with the requested program  
- `wait()` — synchronizes execution with the parent  

Commands such as:

```
ls
cat file.txt
echo hello
```

execute in the foreground through a spawned process.

Arguments are tokenized prior to execution.

---

## Process Listing (`lp`)

The `lp` command provides a basic process viewer by scanning `/proc`.

For each numeric PID entry, Minishell:

- extracts the PID from the directory name  
- reads the command line from `/proc/<pid>/cmdline`  
- retrieves the associated username from system password records  
- stores process entries in an array  
- sorts them numerically  
- prints results in aligned columns  

This demonstrates interaction with:

- the `/proc` virtual filesystem  
- kernel-generated process metadata  
- filesystem traversal and user lookup  

---

## Signal Handling

Minishell registers a custom `SIGINT` handler for `Ctrl-C`.

Behavior:

- foreground child processes receive the interrupt  
- the shell remains active  
- interrupted input lines reset cleanly  
- state is tracked using a `volatile sig_atomic_t` flag  

This allows interruptions without terminating the shell session.

---

## Building

### Compile from source

```bash
gcc minishell.c -o minishell
```

### Run the shell

```bash
./minishell
```
