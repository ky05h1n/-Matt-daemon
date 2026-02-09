<p align="center">
  <img src="https://img.shields.io/badge/Language-C++-blue?style=for-the-badge&logo=cplusplus&logoColor=white" />
  <img src="https://img.shields.io/badge/OS-Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black" />
  <img src="https://img.shields.io/badge/Requires-Root-red?style=for-the-badge&logo=gnubash&logoColor=white" />
  <img src="https://img.shields.io/badge/Port-4242-green?style=for-the-badge" />
</p>

<h1 align="center">🔒 Matt_daemon</h1>

<p align="center">
  <i>A UNIX background daemon that listens on port 4242, logs all incoming messages, and shuts down gracefully on <code>quit</code> or any catchable signal.</i>
</p>

---

## 📦 Build

```bash
make          # 🔨 compile
make clean    # 🧹 remove binary
make re       # 🔄 recompile from scratch
```

---

## 🚀 Quick Start

```bash
# 1. Start the daemon (requires root)
sudo ./Matt_daemon

# 2. Connect to it
nc localhost 4242

# 3. Type anything — it gets logged
Hello
World

# 4. Type quit — daemon shuts down
quit
```

---

## 📋 How It Works

```
┌─────────────────────────────────────────────────────┐
│                   Matt_daemon                       │
│                                                     │
│   1. Check root privileges                          │
│   2. Create lock file  (/var/lock/matt_daemon.lock) │
│   3. Fork → child becomes the daemon                │
│   4. Detach from terminal (setsid)                  │
│   5. Open log file     (/var/log/matt-daemon.log)   │
│   6. Listen on port 4242                            │
│   7. Accept up to 3 clients via select()            │
│   8. Log all input, quit on "quit" or signal        │
│   9. Clean up: close sockets, remove lock file      │
└─────────────────────────────────────────────────────┘
```

---

## 📝 Log File

**Location:** `/var/log/matt-daemon.log`

```log
[09/02/2026-14:30:00] [ INFO ]  - Matt_daemon: Started.
[09/02/2026-14:30:00] [ INFO ]  - Matt_daemon: Creating server.
[09/02/2026-14:30:00] [ INFO ]  - Matt_daemon: Server created.
[09/02/2026-14:30:00] [ INFO ]  - Matt_daemon: Entering Daemon mode.
[09/02/2026-14:30:00] [ INFO ]  - Matt_daemon: started. PID: 1234.
[09/02/2026-14:30:10] [ LOG ]   - Matt_daemon: User input: Hello
[09/02/2026-14:30:12] [ LOG ]   - Matt_daemon: User input: World
[09/02/2026-14:30:15] [ INFO ]  - Matt_daemon: Request quit.
[09/02/2026-14:30:15] [ INFO ]  - Matt_daemon: Quitting.
```

---

## 🔐 Lock File

**Location:** `/var/lock/matt_daemon.lock`

| Event | Action |
|:------|:-------|
| 🟢 Daemon starts | Lock file **created** — prevents duplicate instances |
| 🔴 Daemon stops  | Lock file **deleted** — allows restart |
| ⚠️ Second instance | Logs `Error file locked.` and exits |

```
$ sudo ./Matt_daemon       # ✅ starts
$ sudo ./Matt_daemon       # ❌ Can't open :/var/lock/matt_daemon.lock
```

---

## ⚡ Signal Handling

The daemon catches **all** interceptable signals and shuts down gracefully:

| Signal | Example | Handled? |
|:-------|:--------|:--------:|
| `SIGTERM` | `kill -15 PID` | ✅ |
| `SIGINT`  | `Ctrl+C` | ✅ |
| `SIGQUIT` | `kill -3 PID` | ✅ |
| `SIGHUP` `SIGUSR1` `SIGUSR2` ... | various | ✅ |
| `SIGKILL` | `kill -9 PID` | ❌ _kernel enforced_ |
| `SIGSTOP` | `kill -19 PID` | ❌ _kernel enforced_ |

> 💡 `SIGKILL` and `SIGSTOP` **cannot** be caught by any program — the kernel handles them directly.

---

## 🛡️ Constraints

| Rule | Detail |
|:-----|:-------|
| 👤 Root only | Must run as root (`sudo`) |
| 1️⃣ Single instance | Lock file prevents duplicates |
| 👥 Max 3 clients | Additional connections are rejected |

---

## 🗂️ Project Structure

```
Matt_daemon/
├── 📄 Md_header.hpp      # Header — Tintin_reporter & Atr classes
├── 📄 MattDaemon.cpp      # Entry point (main)
├── 📄 Build_damon.cpp     # Daemon logic — fork, signals, lock file
├── 📄 Socket.cpp          # Server — socket, select loop, clients
├── 📄 Logs.cpp            # Tintin_reporter — timestamped logging
├── 📄 Makefile            # Build rules
└── 📄 README.md           # You are here
```

---

## 🛠️ Usage Examples

<details>
<summary><b>Start & verify</b></summary>

```bash
$ sudo ./Matt_daemon
$ ps aux | grep Matt
root  6498  0.0  0.0  15172  2164 ?  Ss  14:34  0:00  ./Matt_daemon
$ ls -la /var/lock/ | grep matt
-rw-r--r-- 1 root root 0 Feb 09 14:34 matt_daemon.lock
```
</details>

<details>
<summary><b>Send messages</b></summary>

```bash
$ nc localhost 4242
Hello
xd
quit
$ tail -n 5 /var/log/matt-daemon.log
[09/02/2026-14:36:43] [ LOG ]  - Matt_daemon: User input: Hello
[09/02/2026-14:36:44] [ LOG ]  - Matt_daemon: User input: xd
[09/02/2026-14:36:47] [ INFO ] - Matt_daemon: Request quit.
[09/02/2026-14:36:47] [ INFO ] - Matt_daemon: Quitting.
```
</details>

<details>
<summary><b>Kill with signal</b></summary>

```bash
$ sudo kill -15 6498
$ tail -n 2 /var/log/matt-daemon.log
[09/02/2026-14:35:24] [ INFO ] - Matt_daemon: Signal handler.
[09/02/2026-14:35:24] [ INFO ] - Matt_daemon: Quitting.
$ ls /var/lock/ | grep matt    # lock file removed ✅
```
</details>

---

<p align="center">
  <sub>42 Project — UNIX Matt_daemon</sub>
</p>
