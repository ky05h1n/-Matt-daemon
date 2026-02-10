# Bonus: ROT13 Cipher + Authentication

This bonus uses ROT13 for encrypted mode and adds a password-based authentication step.

## Build

```bash
make bonus
```

## Run

**Plain mode (no key):**
```bash
export MD_AUTH_PASSWORD_CLIENT_1=secret123
export MD_AUTH_PASSWORD_CLIENT_2=shihaja123
export MD_AUTH_PASSWORD_CLIENT_3=kawkawka654
./Matt_daemon_bonus
./Ben_AFK 127.0.0.1
```

**Encrypted mode (ROT13):**
```bash
export MD_AUTH_PASSWORD_CLIENT_1=secret123
export MD_AUTH_PASSWORD_CLIENT_2=shihaja123
export MD_AUTH_PASSWORD_CLIENT_3=kawkawka654
./Matt_daemon_bonus --encrypted myKey
./Ben_AFK 127.0.0.1 --encrypted myKey
```

## Protocol

1. Client sends `ENC` + key (encrypted mode) or `PLAIN` (plain mode).
2. Daemon replies `MODE:ROT13` or `MODE:PLAIN` (plain).
3. Daemon sends `Enter Your Password` using selected mode.
4. Client sends password using selected mode.
5. Daemon replies `AUTH_OK` or `AUTH_FAIL`.
6. After `AUTH_OK`, messages use ROT13 if mode is `ROT13`, otherwise plaintext.
7. `quit` stops the daemon.

## Notes

- If daemon runs with `--encrypted`, client must also use `--encrypted` and the same key.
- If daemon runs without `--encrypted`, client must not send a key (plain mode only).
- Password failures are logged to `/var/log/matt-daemon.log`.
- Daemon requires root (for log file access).

## Mail bonus (bonus daemon)

`Matt_daemon_bonus` also sends email using the same SMTP env vars from `README_MAIL_BONUS.md`.

To receive emails for normal messages, set:

```bash
export MD_SMTP_MIN_LEVEL=LOG
```

`AUTH` failures are sent as `ERROR`.
