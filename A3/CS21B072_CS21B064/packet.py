# packet.py

import globals

def make_packet(seq_num, data):
    # no. of bytes used for representation
    seq_bytes = seq_num.to_bytes(1, byteorder='big')
    return seq_bytes + data

# packet in bytes
def generate_packets(file, packet_size_b):
    packets = []
    seq_num = 0
    while True:
        data = file.read(packet_size_b)
        if not data:
            break
        packets.append(make_packet(seq_num, data))
        seq_num = (seq_num + 1) % globals.max_seq_num
    return packets

def extract_packet(packet):
    seq_num = int.from_bytes(packet[0:1], byteorder='big')
    return seq_num, packet[1:]