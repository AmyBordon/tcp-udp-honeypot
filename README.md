# TCP/UDP Honeypot with Real-Time Attack Detection

A honeypot written in C that simulates exposed network services, records what
attackers send, and detects four types of attack in real time. Built for a
computer networks course.

## Attacks detected

| Family | Attack | Protocol | How it is detected |
|---|---|---|---|
| Authentication | Brute-force | TCP | same IP, same user, many different passwords |
| Authentication | Credential stuffing | TCP | same IP, many different users, 1-2 attempts each |
| DDoS | Amplification (DNS ANY, NTP monlist) | UDP | 100+ identical packets in 10 s, recognized from the first 64 bytes |
| DDoS | Flood | UDP | 100+ packets in 10 s that are not a known amplification request |

<img width="615" height="339" alt="image" src="https://github.com/user-attachments/assets/6f697898-6279-49ca-a126-ad15732efdd6" />


## Architecture

- **TCP server** (port 23): concurrent (`fork` per client). Shows an Ubuntu
  banner, asks for `login:` / `password:`, always answers "Incorrect login" and
  disconnects after 3 attempts. Every attempt is logged. It reads input up to
  `\n`, since TCP does not preserve message boundaries.
- **UDP server** (port 5353): iterative, because UDP is connectionless and each
  datagram is handled quickly. It never replies, so it cannot be abused as a
  reflector. It logs the source IP, packet size and the first 64 bytes as hex.
- **Monitor**: a separate process that reads both logs every 10 s and keeps
  per-IP counters. It emits `[ALERT]` when an attack starts, `[UPDATE]` while it
  continues and `[ENDED]` after a period of silence. Alerts go to the terminal
  (colored) and to `alerts.txt`.
- **Python clients**: simulate brute-force, credential stuffing, DNS ANY, NTP
  monlist and flood traffic, for testing on localhost or across a LAN.

## Build and run

    make run      # compile and start both servers and the monitor
    make stop     # stop everything and delete the logs
    make fresh    # clean restart

Client examples:

    cd tcp && python3 client.py
    cd udp && python3 client.py

## Design notes

- One alert per attack episode, not per packet: an episode ends after a quiet
  period, and a new alert appears only if the attack type changes or the IP
  returns later.
- TCP source IPs are reliable because of the three-way handshake. UDP source
  IPs can be spoofed, so in amplification alerts the IP may be the victim.
- The UDP server can drop packets under flood when the kernel receive buffer
  fills up, so counts are a lower bound.

## Limitations

- Thresholds are heuristics.
- The TCP service mimics a login prompt, not the full Telnet protocol.
- Slow attacks (below the thresholds) are not detected.
- Distributed attacks from many IPs are not correlated.
- Port scans are not detected.

## Possible improvements

HTTPS or SSH service, per-IP rate limiting, scan detection, correlation between
TCP and UDP activity, running on a VPS and publishing statistics.

## Disclaimer

For educational use. Run it only on networks you own or are allowed to test,
ideally isolated, and never publish raw logs (they may contain real passwords).
