import socket, struct

IP = "172.30.245.124"
PORT = 5353
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

ntp = bytes([0x17, 0x00, 0x03, 0x2a]) + b"\x00" * 4
dns = (struct.pack(">HHHHHH", 0x1234, 0x0100, 1, 0, 0, 0)
       + b"\x07example\x03com\x00"
       + struct.pack(">HH", 255, 1))

TIP = "ntp"     # "ntp", "dns" sau "flood"
for i in range(200):

    if TIP == "ntp":
        s.sendto(ntp, (IP, PORT))
    elif TIP == "dns":
        s.sendto(dns, (IP, PORT))
    else:
        s.sendto(b"flood", (IP, PORT))
s.close()
