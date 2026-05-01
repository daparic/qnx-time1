# time1

QNX Neutrino example of a message-passing server and client pair. What full video [here](https://youtu.be/Sr75ilk1UN4).

- **`time1`** — [server](https://www.qnx.com/developers/docs/8.0/com.qnx.doc.neutrino.getting_started/topic/examples.html) that receives periodic 1 Hz timer pulses and blocking
  messages from clients, demonstrating `MsgReceive`/`MsgReply` with
  `timer_create`.
- **`another`** — A new client that I created that connects to the server by PID and channel ID,
  sends either a `wait` or `send` message, and prints the server's reply.

## How it works

### Server (`time1`)

On startup `time1`:
1. Creates a channel with `ChannelCreate` and prints its channel ID (`chid`).
2. Creates a 1 Hz periodic timer that delivers a pulse to that channel every
   second.
3. Enters a `MsgReceive` loop.  `MsgReceive` returns:
   - `rcvid == 0` — a timer pulse arrived → `gotAPulse`
   - `rcvid > 0`  — a client called `MsgSend` → `gotAMessage`

### Client messages (`another`)

Two message types are defined in `protocol.h`:

| Type | Value | Meaning |
|------|-------|---------|
| `MT_WAIT_DATA` | 2 | Client wants to wait for data from another client |
| `MT_SEND_DATA` | 3 | Client is delivering data |

Two reply types:

| Type | Value | Meaning |
|------|-------|---------|
| `MT_OK` | 0 | Data was exchanged successfully |
| `MT_TIMEDOUT` | 1 | Wait client timed out before a sender arrived |

### Wait / send rendezvous

A `wait` client calls `MsgSend(MT_WAIT_DATA)` and **blocks** (REPLY-blocked in
the kernel) until either:
- A `send` client arrives within the timeout window → both clients receive
  `MT_OK` simultaneously.
- The timeout expires → the wait client receives `MT_TIMEDOUT`.

The server never calls `MsgReply` for a `wait` client immediately; it saves the
`rcvid` in a table and replies later. A `send` client that finds no waiting
client is **never replied to** in the current implementation, so it blocks
indefinitely until a `wait` client registers. Always start a `wait` client
before a `send` client.

### Shutdown sentinel (data = 42)

When an `MT_SEND_DATA` message carries `messageData == 42` the server treats it
as a shutdown sentinel. It replies `MT_OK` to unblock the sender and then calls
`exit(EXIT_SUCCESS)`. The reply-before-exit ordering is intentional — without
it the sender would be left permanently REPLY-blocked with no process to
unblock it.

### Timeout duration

The `wait` client's timeout is a **tick counter** initialised to `timeout = 5`
(in `time1.c`). The 1 Hz timer decrements every registered waiter by 1 on each
pulse. When the counter reaches 0 the server sends `MT_TIMEDOUT`.

Because the timer runs continuously from server startup, a freshly registered
client may lose part of the first tick. The actual wait is in the range:

```
(timeout - 1, timeout]  seconds
```

With the default `timeout = 5` the client waits between **4 and 5 seconds**.
Changing it to 60 produces a wait of **59–60 seconds**, not exactly 60.

To guarantee a minimum of N seconds, set `timeout = N + 1`.

## Prerequisites

- QNX SDP 7.x or 8.x installed on the build host
- CMake 3.21 or later
- [Ninja](https://ninja-build.org/) build tool (bundled with many QNX SDPs under `$QNX_HOST/usr/bin`)
- The QNX SDP environment script sourced in your shell

> **Windows note:** CMake on Windows defaults to the Visual Studio generator,
> which does not support cross-compilation toolchain files. The
> `CMakePresets.json` in this project explicitly selects **Ninja**, bypassing
> that default. Make sure `ninja.exe` is on your `PATH` (the QNX SDP ships one
> at `%QNX_HOST%\usr\bin\ninja.exe`).

## Environment setup

Source the QNX SDP environment script before building. It sets `QNX_HOST`,
`QNX_TARGET`, and adds the QNX toolchain to `PATH`.

**Linux host:**
```bash
source <sdp-install-dir>/qnxsdp-env.sh
```

**Windows host (Command Prompt):**
```cmd
<sdp-install-dir>\qnxsdp-env.bat
```

Verify the variables are set:

```bash
# Linux / Git Bash
echo $QNX_HOST
echo $QNX_TARGET

# Windows Command Prompt
echo %QNX_HOST%
echo %QNX_TARGET%
```

## Build (using presets — recommended)

```bash
# Configure + build (Release)
cmake --preset qnx-x86_64-release
cmake --build --preset qnx-x86_64-release
```

Both `time1` and `another` are placed in `build/`.

### Debug build

```bash
cmake --preset qnx-x86_64-debug
cmake --build --preset qnx-x86_64-debug
```

Binaries are placed in `build-debug/`.

### Building a single target

The configure step always processes the full `CMakeLists.txt`. The build step
accepts an optional `--target` flag to compile and link only one binary:

```bash
cmake --build build --target time1
cmake --build build --target another
```

## Build (manual — alternative)

If you prefer to invoke CMake directly, pass `-G Ninja` explicitly so CMake
does not fall back to Visual Studio on Windows:

```bash
cmake -B build -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-qnx-x86_64.cmake \
      -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

To build a single target with the manual approach:

```bash
cmake --build build --target time1
```

## Deploying to a QNX target

```bash
scp build/time1 build/another user@<target-ip>:/tmp/
ssh user@<target-ip>
```

## Test workflows

All commands below are run on the QNX target. Open three terminal sessions
(e.g. three SSH connections or three `slay`/`screen` windows).

### 1. Start the server

```
Terminal 1:  $ /tmp/time1
             chid = 1
             Got a Pulse at Thu May  1 ...
             Got a Pulse at Thu May  1 ...
```

Note the printed `chid` value and the server's PID:

```bash
pidin -p time1   # shows PID in the second column
```

All `another` invocations below use `<pid>` and `<chid>` from these values.

### 2. Workflow A — wait client times out (no sender)

Run a single `wait` client and let it expire.

```
Terminal 2:  $ /tmp/another <pid> <chid> wait
             Sending MT_WAIT_DATA to pid=<pid> chid=<chid> ...
```

The client blocks.  After **4–5 seconds** (one partial tick plus four full
ticks, with the default `timeout = 5`):

```
             Reply: MT_TIMEDOUT
             $
```

### 3. Workflow B — successful rendezvous between wait and send

Start a `wait` client, then within the timeout window send data to it.

```
Terminal 2:  $ /tmp/another <pid> <chid> wait
             Sending MT_WAIT_DATA to pid=<pid> chid=<chid> ...
             (blocks)

Terminal 3:  $ /tmp/another <pid> <chid> send 777
             Sending MT_SEND_DATA data=777 to pid=<pid> chid=<chid> ...
```

Both clients unblock simultaneously:

```
Terminal 2:  Reply: MT_OK  data=777
Terminal 3:  Reply: MT_OK  data=777
```

### 4. Workflow C — send without a prior wait (broken ordering)

Starting `send` before `wait` leaves the send client blocked indefinitely
because the server never replies to a sender when the waiter table is empty:

```
Terminal 2:  $ /tmp/another <pid> <chid> send 99
             Sending MT_SEND_DATA data=99 to pid=<pid> chid=<chid> ...
             (blocks forever — Ctrl-C to abort)
```

The server prints:

```
             Table empty, message from rcvid <n> ignored, client blocked
```

Always start the `wait` client before the `send` client.

### 5. Workflow D — shutting down the server (data = 42)

Send the shutdown sentinel from any terminal. No prior `wait` client is needed.

```
Terminal 2:  $ /tmp/another <pid> <chid> send 42
             Sending MT_SEND_DATA data=42 to pid=<pid> chid=<chid> ...
             Reply: MT_OK  data=42
             $
```

The server prints and exits:

```
Terminal 1:  time1.c:  received shutdown sentinel (42), terminating
             $
```

## Generating a Parasoft BDF

A Parasoft Build Data File (BDF) records every compiler invocation so that
Parasoft C/C++test can locate sources, headers, and flags without re-running
the build itself.

### Why a clean build is required

Ninja performs incremental builds: if an object file is already up to date it
skips the compilation step entirely. `cpptesttrace` can only capture commands
that actually execute during the traced build. A stale `build/` directory means
some or all translation units are skipped, producing an **incomplete BDF** that
is missing source files and include paths.

**Always remove `build/` before tracing** to guarantee every compilation
command is executed and recorded.

### Steps

```bash
# 1. Remove any existing build directory
rm -rf build/

# 2. Configure (generates build.ninja — no compilation yet)
cmake -B build -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-qnx-x86_64.cmake \
      -DCMAKE_BUILD_TYPE=Release

# 3. Trace the build for time1 only
cpptesttrace ninja -C build time1
```

`cpptesttrace` wraps Ninja, intercepts every compiler call, and writes the BDF
(typically `build/time1.bdf` or `cpptesttrace.bdf` depending on your Parasoft
installation). Refer to your C/C++test documentation for the exact output path
and any `-bdf` flag options.

To trace both targets in one pass omit the target name:

```bash
cpptesttrace ninja -C build
```

## Project structure

```
time1/
├── CMakeLists.txt                  # CMake build definition
├── CMakePresets.json               # Build presets (enforces Ninja generator)
├── cmake/
│   └── toolchain-qnx-x86_64.cmake # QNX x86_64 cross-compilation toolchain
├── protocol.h                      # Shared message type definitions
├── time1.c                         # Server
└── another.c                       # Client
```
