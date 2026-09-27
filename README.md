# cshell — A Custom UNIX-style Shell

A shell implemented in C from scratch, built incrementally across a sequence of
feature specs (input/output redirection, piping, sequencing, background jobs,
job control, and a set of custom introspection utilities). This README
summarizes what was built and the real issues encountered along the way.

---

## 1. Building and Running

### Prerequisites

The shell is written in C and uses POSIX system calls and Linux-specific interfaces such as `/proc` and `ptrace`. It is therefore intended to be built and run in a Linux environment.

You should have:
- `gcc`
- `make`
- A Linux/POSIX environment

### Build

From the project root, run:

```bash
make all
```

This builds the project and creates the executable:

```text
shell.out
```

### Run

After a successful build, start the shell with:

```bash
./shell.out
```

The shell will start its REPL and display its prompt. You can then enter supported shell commands and builtins.

### Clean Build Artifacts

If the project Makefile provides a `clean` target, build artifacts can be removed with:

```bash
make clean
```

Then rebuild using:

```bash
make all
```

## Project Overview

`cshell` is a custom UNIX-style shell implemented in C from scratch. It provides
a command-line REPL together with command execution, redirection, pipelines,
sequencing, background execution, job control, and custom process/file
introspection utilities.

The normal workflow is:

```text
make all
   ↓
shell.out is created
   ↓
./shell.out
   ↓
Enter commands in the custom shell
```

## 2. Architecture Overview

| File(s) Responsibility      |                                                                                                                                                                                                                                                         |
| --------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `lexer.c` / `lexer.h`       | Tokenizes raw input into `Token`s (`WORD`, `OP_PIPE`, `OP_LT`, `OP_GT`, `OP_GTGT`, `OP_AMP`, `OP_SEMI`). Handles quoting (`'…'`, `"…"`) and backslash escaping.                                                                                         |
| `parser.c` / `parser.h`     | Recursive-descent grammar validator confirming a token stream is syntactically valid before anything executes.                                                                                                                                          |
| `main.c`                    | The REPL: prompt formatting, reads a line, lexes/parses it, and dispatches to a builtin or to command execution.                                                                                                                                        |
| `builtins.c` / `builtins.h` | Wraps `hop`, `reveal`, `peek`, `locate` behind a uniform `is_builtin()` / `run_builtin()` interface so they can run either in-process (simple case) or inside a forked child (when piped, redirected, or backgrounded).                                 |
| `hop.c`, `frecency.c`       | `cd`-like directory navigation with a persistent, frecency-scored jump list (`~/.hop_history.log`).                                                                                                                                                     |
| `reveal.c`                  | `ls`-like directory listing (`-a` hidden entries, `-t` recursive tree).                                                                                                                                                                                 |
| `peek.c`                    | `cat`-like file/stdin viewer (`-n` line numbering, `-r` reverse, seek-based reverse reading for regular files).                                                                                                                                         |
| `locate.c`                  | Finds executables by name in the cwd and `$PATH`.                                                                                                                                                                                                       |
| `exec.c`                    | Resolves a command name to a real path (`resolve_command_path`, handling `%`-prefixed PATH-only lookups) and runs a single non-piped command.                                                                                                           |
| `redirect.c`                | `input_redirection()` — concatenates one or more `<` files into an anonymous temp stream for a command's stdin.                                                                                                                                         |
| `output_redirect.c`         | `open_output_files()` / `close_fds()` — opens multiple `>` / `>>` targets, each with its own truncate/append mode.                                                                                                                                      |
| `pipeline.c`                | The core execution engine: builds an N-stage pipeline, wires `pipe()`s between stages, forks each stage, applies each stage's own redirection, sets up process groups, and (for foreground jobs) manages terminal control and stop/interrupt detection. |
| `seq_exec.c`                | Splits a command line on `;` and `&`, and for each segment further splits on \`                                                                                                                                                                         |
| `background.c`              | Job table (`jobs[]`), `SIGCHLD` handler (async, non-blocking reaping via `WNOHANG`), job numbering, deferred/immediate completion reporting.                                                                                                            |
| `terminal.c`                | Shell-side `SIGINT`/`SIGTSTP` handlers, `tcsetpgrp()`-based terminal handoff/reclaim, stopped-job printing, `SIGHUP`-on-exit cleanup.                                                                                                                   |
| `resume.c`                  | \`resume %n [fg                                                                                                                                                                                                                                         |
| `ping.c`                    | Sends an arbitrary signal (`signal_number % 64`) to a pid or an entire job's process group.                                                                                                                                                             |
| `spy.c`                     | Lists a process's open files via `/proc/<pid>/{cwd,exe,maps,fd}`.                                                                                                                                                                                       |
| `snoop.c`                   | Traces a command's syscalls via `ptrace`, reporting a per-syscall count/time summary.                                                                                                                                                                   |

---

## 3. Features Implemented

### C — Command Execution Core

- **C1 — Command resolution & exec:** slash-path vs. bare-name lookup, cwd-then-`$PATH` search order, `%`-prefix to force a PATH-only lookup (skipping the cwd check).
- **C2 — Input redirection (`<`):** multiple `<` files concatenated, in order, into one continuous stream via a self-cleaning anonymous temp file (`mkstemp` + immediate `unlink`).
- **C3 — Output redirection (`>` / `>>`):** any number of output targets per command, each independently truncating or appending; a command's stdout is fanned out to every target via a pipe-and-tee, since a single `dup2` can't target more than one fd.
- **C4 — Piping (`|`):** full N-stage pipelines — one `pipe()` per stage boundary, one fork per stage, correct fd wiring and closing in both parent and children, "command not found" on one stage doesn't kill the rest of the pipeline.

### D — Sequencing & Backgrounding

- **D1 — Sequential execution (`;`):** each segment runs to completion before the next starts; a segment that fails to resolve a command halts the remaining sequence.
- **D2 — Background execution (`&`):** each `&`-segment forks and returns control to the prompt immediately; job numbers are monotonically increasing and never reused; completion is detected asynchronously via a `SIGCHLD` handler using `waitpid(-1, …, WNOHANG)`, with reporting deferred while a foreground command is active and flushed right after it finishes.

### E — Job Control

- **E1 — `activities`:** lists every process the shell has spawned that is still running, grouped by process group, showing pid/name/state (`Running`/`Stopped`).
- **E2 — Terminal control (Ctrl-C / Ctrl-Z / Ctrl-D):** the shell installs no-op `SIGINT`/`SIGTSTP` handlers so it survives them; `tcsetpgrp()` hands the terminal to a foreground job's process group before running it and reclaims it afterward; Ctrl-Z is detected via `waitpid(…, WUNTRACED)` and registers the job as `Stopped`; Ctrl-D warns instead of exiting while a stopped job exists, and exits (with `SIGHUP` sent to every remaining job) on a second consecutive press.
- **E3 — `resume`:** brings a stopped or backgrounded job back to `Running` via `SIGCONT`, either in the foreground (blocking, with an optional `--timeout`) or in the background.
- **E4 — `ping`:** delivers an arbitrary signal to a specific tracked pid, or to an entire job's process group via a `%`-prefixed job number.

### F — Introspection Utilities

- **F1 — `spy`:** reports a process's `cwd`, executable (`txt`), memory-mapped files (`mem`), and open file descriptors (`fd`), reading directly from `/proc`.
- **F2 — `snoop`:** traces either a freshly-launched command or an already-running pid (`-p`) via `PTRACE_SYSCALL`, timing each syscall and printing a summary table sorted by call count (ties broken by first occurrence), with unrecognized syscalls falling back to a numbered `syscall_N` label.

---

## 4. Notable Challenges & Bugs Encountered

Building this iteratively surfaced a long list of real bugs — most only visible
once actually run, not from reading the code. The recurring theme: **most bugs
were caught by testing, not by static review**, which shaped how the project
was iterated on.

- **Stale argument counts.** An early version passed the *total* token count (including `<`/`>` operators and their filenames) into `execute_command` instead of the *actual* argument count built for that command — this left uninitialized stack memory in the `argv` array passed to `execv`, producing `execv: Bad address` crashes.
- **A single flipped comparison (`>=0` vs. `>0`)** in a cleanup path caused `close()` to be called on garbage stack values whenever a command had no output redirection — which occasionally closed the shell's own `stdin`, causing an infinite prompt-reprint loop after the *next* command ran.
- **Signal-handler correctness.** The `SIGCHLD` handler originally called `printf` directly — not async-signal-safe — which could corrupt `stdio`'s internal buffering state and crash the shell later, at a seemingly unrelated point. Rewritten to build the message with `snprintf` and emit it with the async-signal-safe `write()`.
- **A genuine race between two `waitpid` callers.** Both the foreground execution path and the async `SIGCHLD` handler were calling `waitpid` on the same children without coordination — occasionally "stealing" each other's status notifications. Fixed by blocking `SIGCHLD` around the foreground `waitpid` loop, making it the sole reaper for that window.
- **Missing terminal handoff.** Ctrl-C/Ctrl-Z appeared to do nothing (the child ran for its full duration regardless) because `tcsetpgrp()` was never actually being called to give the terminal to the job's process group — the shell itself remained the foreground process group the whole time, so the signals were delivered to the (intentionally no-op) shell handlers instead of the child.
- **Job-numbering violated "never reused."** An intermediate version searched for the *smallest available* job number, which reused numbers once earlier jobs finished — directly against the spec ("never reused, even after a job completes"). Fixed to always draw from a monotonically increasing counter.
- **Ordering guarantee for background jobs.** A job's `[n] pid` line has to print *before* any output the job itself produces — but nothing enforced that ordering, so a fast command like `echo hello &` could race ahead of the parent's own print statement. Fixed with a synchronization pipe: every backgrounded child blocks on a `read()` immediately after `fork()`, and the parent releases all of them (by closing the pipe) only after the job line has been printed.
- **Builtins bypassing the entire pipeline machinery.** `hop`, `reveal`, `peek`, and `locate` were originally special-cased directly in `main.c`, completely outside of `pipeline.c` — meaning they silently couldn't be piped, redirected, or backgrounded at all. Restructured behind a shared `is_builtin()`/`run_builtin()` interface so a *simple* invocation still runs in-process (so `hop` can actually change the shell's own directory), while any piped/redirected/backgrounded invocation now correctly forks and runs the builtin in a child — matching the expected behavior that a backgrounded `hop` should not affect the parent shell's working directory.
- **Fixed-size lexer buffer overflow.** The word-token buffer was a 256-byte stack array with no bounds check — an unbroken input word longer than that (e.g. a long string with no spaces) silently overflowed it and corrupted the stack. Fixed with an explicit bounds check that rejects an oversized word with a syntax error instead of overflowing, and the buffer was later sized to match the shell's own maximum input length (1024 bytes).
- **`ptrace`-attached processes get "stolen" from the job table.** When `snoop -p <pid>` attaches to an already-tracked background job, the traced process's *own* exit is reported to the tracer (`snoop`), not to the shell's own `SIGCHLD` handler — meaning the job's normal "exited normally" message would silently never appear. Fixed by having `snoop` explicitly notify the job table once its own trace loop observes the target's exit.
- **Environment/build quirks specific to this setup:** several POSIX functions (`realpath`, `sigaction`/`SA_RESTART`, `tcsetpgrp`, `getpgrp`) needed `_POSIX_C_SOURCE` defined *before* the first system header is included in a translation unit — defining it after `main.h`'s own includes had already pulled in `<stdlib.h>`/`<unistd.h>` silently had no effect, which took several iterations to pin down precisely.

---


## 6. Commands

The shell supports standard external commands through `exec`, along with the
following custom builtins and utilities.

### Directory Navigation

#### `hop`

Navigate between directories using the shell's frecency-based directory history.

```text
hop
hop ~
hop ..
hop -
hop <path>
```

- `hop` — go to the home directory.
- `hop ~` — go to the home directory.
- `hop ..` — move to the parent directory.
- `hop -` — return to the previous directory.
- `hop <path>` — navigate to the specified path.
- Directory usage is persisted in `~/.hop_history.log`.

### File and Directory Inspection

#### `reveal`

List directory contents.

```text
reveal
reveal -a
reveal -t
reveal <path>
reveal -a <path>
reveal -t <path>
```

- `-a` — include hidden entries.
- `-t` — recursively display the directory tree.
- A path may be supplied to inspect a specific directory.

#### `peek`

Display file contents or read from standard input.

```text
peek <file>
peek -n <file>
peek -r <file>
peek -n -r <file>
```

- `-n` — display line numbers.
- `-r` — display the contents in reverse order.
- Regular files can be read using seek-based reverse reading.

#### `locate`

Find an executable by name.

```text
locate <command>
```

The lookup searches the current working directory and `$PATH`.

### Job Control

#### `activities`

List processes spawned by the shell that are still running or stopped.

```text
activities
```

The output includes the process ID, command name, and state.

#### `resume`

Resume a stopped or backgrounded job.

```text
resume %<job_number>
resume %<job_number> fg
resume %<job_number> bg
resume %<job_number> fg --timeout <seconds>
```

A `%` prefix identifies a shell job number.

#### `ping`

Send a signal to a tracked process or process group.

```text
ping <signal_number> <pid>
ping <signal_number> %<job_number>
```

The `%` form targets the process group associated with the specified job.

### Process Introspection

#### `spy`

Inspect information about a process through `/proc`.

```text
spy <pid>
```

The utility can report information such as:

- Current working directory (`cwd`)
- Executable (`txt`)
- Memory-mapped files (`mem`)
- Open file descriptors (`fd`)

#### `snoop`

Trace system calls made by a command or an already-running process.

```text
snoop <command> [arguments...]
snoop -p <pid>
```

The output provides a syscall count/time summary. When tracing an existing
process, `-p` specifies its PID.

### Shell Operators

The shell also supports the following operators:

| Operator | Purpose | Example |
|---|---|---|
| `;` | Sequential execution | `cmd1 ; cmd2` |
| `&` | Background execution | `cmd &` |
| `\|` | Pipe output between commands | `cmd1 \| cmd2` |
| `<` | Input redirection | `cmd < input.txt` |
| `>` | Output redirection | `cmd > output.txt` |
| `>>` | Append output | `cmd >> output.txt` |

Multiple redirections and pipeline stages can be combined where supported by
the shell grammar.

### Job Control Keys

The shell handles the following terminal controls:

| Key | Behaviour |
|---|---|
| `Ctrl-C` | Interrupt the foreground job |
| `Ctrl-Z` | Stop the foreground job |
| `Ctrl-D` | Warns when stopped jobs exist; otherwise participates in shell exit handling |


## 7. Testing Approach

Each feature was validated against its own literal spec example first, then
against deliberately adversarial cases: multiple redirections and pipelines
combined, command-not-found mid-sequence, rapid signal delivery, jobs
finishing while a different foreground command is running, and empty/oversized
input. Several of the fixes above were only discovered because a spec example
was tested exactly and its output compared line-by-line rather than "looks
about right."