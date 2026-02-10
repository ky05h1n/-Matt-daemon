# Mail Bonus (SMTP)

This bonus sends an email when a log entry matches filters.

## Configuration (environment variables)

- `MD_SMTP_HOST` (required): SMTP server hostname or IP.
- `MD_SMTP_PORT` (optional, default `25`).
- `MD_SMTP_HELO` (optional, default `localhost`).
- `MD_SMTP_USER` / `MD_SMTP_PASS` (optional): enable `AUTH LOGIN` if both are set.
- `MD_SMTP_FROM` (optional, default `matt-daemon@localhost`).
- `MD_SMTP_TO` (required): comma-separated list of recipients.
- `MD_SMTP_SUBJECT` (optional, default `matt-daemon alert`).
- `MD_SMTP_TLS` (optional): `starttls`, `ssl`/`smtps`, or empty for plain.
- `MD_SMTP_MIN_LEVEL` (optional, default `ERROR`): `INFO`, `LOG`, or `ERROR`.
- `MD_SMTP_KEYWORDS` (optional): comma-separated keywords; if set, message must contain one.

## Usage

```bash
export MD_SMTP_HOST=127.0.0.1
export MD_SMTP_PORT=25
export MD_SMTP_TO=admin@example.com
export MD_SMTP_MIN_LEVEL=ERROR
export MD_SMTP_KEYWORDS=panic,failed

make
sudo ./Matt_daemon
```


## Gmail example (App Password)

```bash
export MD_SMTP_HOST=smtp.gmail.com
export MD_SMTP_PORT=587
export MD_SMTP_TLS=starttls
export MD_SMTP_USER=your@gmail.com
export MD_SMTP_PASS=your_app_password
export MD_SMTP_FROM=your@gmail.com
export MD_SMTP_TO=your@gmail.com
```

## Notes

- This uses plain SMTP over TCP (no TLS). For TLS, run a local SMTP relay like Postfix or stunnel.
- Filters are applied to the original log message (before the timestamp).
