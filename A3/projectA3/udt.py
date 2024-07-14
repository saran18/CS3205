# udt.py - Unreliable data transfer using UDP
import random
import socket
import packet

DROP_PROB = 1

# Send a packet across the unreliable channel
# Packet may be lost
def send(packet1, sock, addr):
    # if random.randint(0, DROP_PROB) > 0:
    sock.sendto(packet1, addr)
    # else:
    #     seq_num, _ = packet.extract_packet(packet1)
    #     print('Dropped packet by probab ' + str(seq_num) + '!')
    return

# Receive a packet from the unreliable channel
def recv(sock):
    packet, addr = sock.recvfrom(100000)
    return packet, addr
