# Matt_daemon

A UNIX daemon that runs in the background and listens for client connections on port **4242**. Messages received are logged, and sending `quit` shuts the daemon down cleanly.

## Build

```bash
make        # compile
make clean  # remove binary
make re     # recompile
```

## Usage

```bash
sudo ./Matt_daemon
```

> **Root privileges are required.** The daemon will refuse to start without them.

### Connect to the daemon

```bash
nc localhost 4242
```

Any text you type is logged. Type `quit` to shut the daemon down.

### Check the daemon is running

```bash
ps aux | grep Matt_daemon
```

### Stop the daemon

```bash
# Option 1: send quit via netcat
echo "quit" | nc localhost 4242

# Option 2: send a signal
sudo kill -15 <PID>
```

## Log file

All activity is logged with timestamps to:

```
/var/log/matt-daemon.log
```

Example output:
```
[09/02/2026-14:30:00] [ INFO ] - Matt_daemon: Started.
[09/02/2026-14:30:00] [ INFO ] - Matt_daemon: Creating server.
[09/02/2026-14:30:00] [ INFO ] - Matt_daemon: Server created.
[09/02/2026-14:30:00] [ INFO ] - Matt_daemon: Entering Daemon mode.
[09/02/2026-14:30:00] [ INFO ] - Matt_daemon: started. PID: 1234.
[09/02/2026-14:30:10] [ LOG ] - Matt_daemon: User input: Hello
[09/02/2026-14:30:15] [ INFO ] - Matt_daemon: Request quit.
[09/02/2026-14:30:15] [ INFO ] - Matt_daemon: Quitting.
```

## Lock file

A lock file is created at:

```
/var/lock/matt_daemon.lock
```

- Created when the daemon starts — prevents multiple instances from running.
- Deleted when the daemon shuts down.
- If a second instance is launched while one is already running:
  ```
  Can't open :/var/lock/matt_daemon.lock, Daemon already running...
  ```

## Signal handling

The daemon intercepts all catchable UNIX signals (SIGINT, SIGTERM, SIGQUIT, etc.), logs the event, cleans up (closes sockets, removes lock file), and exits gracefully.

Signals that **cannot** be caught (enforced by the kernel):
- **SIGKILL (9)** — immediate termination
- **SIGSTOP (19)** — unconditional process stop

## Constraints

- Maximum **3 simultaneous client connections**.
- Only runs as **root**.
- Only **one instance** can run at a time.

## Files

| File | Description |
|---|---|
| `Md_header.hpp` | Header — classes `Tintin_reporter` and `Atr` |
| `MattDaemon.cpp` | Entry point (`main`) |
| `Build_damon.cpp` | Daemon logic — fork, signals, lock file |
| `Socket.cpp` | Server — socket, select loop, client handling |
| `Logs.cpp` | `Tintin_reporter` — timestamped logging |
| `Makefile` | Build rules |
