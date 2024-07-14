# udp_server.py
import socket
import sys
import packet
import globals
import _thread
import udt
import time

from timer import Timer 

DEBUG = 0

RECEIVER_ADDR = ('10.42.80.99', 8080)
SENDER_ADDR = ('10.42.82.171', 0)
TIMEOUT_INTERVAL = 0.5

base = 0
packet_size = None
window_size = None
timeout_interval = None
send_timer = Timer(TIMEOUT_INTERVAL)
mutex = _thread.allocate_lock()

def send_sr(sock, filename):

    global base
    global mutex

    try:
        file = open(filename, 'rb')
    except IOError:
        print('Unable to open', filename)
        return
    
    packets = packet.generate_packets(file, packet_size)

    num_packets = len(packets)
    if(DEBUG): print("total packets: " + str(num_packets))

    _thread.start_new_thread(receive, (sock,))

    base = 0
    next_to_send = 0
    packet_ind = 0

    while base < num_packets:
        mutex.acquire()
        # base is limited to maxseqnum. next_to_send is inc. contin.
        while(next_to_send < base + window_size and next_to_send < num_packets):
            realsno, _ = packet.extract_packet(packets[next_to_send])
            if(DEBUG):print('sending packet no: ' + str(next_to_send) + ' with seq_num: ' + str(realsno))
            udt.send(packets[next_to_send], sock, RECEIVER_ADDR)
            next_to_send += 1

        if not send_timer.running():
            send_timer.start()

        while send_timer.running() and not send_timer.timeout():
            mutex.release()
            # print('Sleeping')
            time.sleep(0.05)
            mutex.acquire()

        if send_timer.timeout():
            if(DEBUG):print('Timeout, resending packets')
            # incase of timeout, we need to resend only the base packet
            udt.send(packets[base], sock, RECEIVER_ADDR)
            if(DEBUG):print("printing after sending timeout packet - " , base)
            # next_to_send = base
            send_timer.stop()
        
        mutex.release()

    udt.send(b'', sock, RECEIVER_ADDR)
    file.close()


def send(sock, filename):
    global base 
    global mutex

    try:
        file = open(filename, 'rb')
    except IOError:
        print('Unable to open', filename)
        return
    
    packets = packet.generate_packets(file, packet_size)

    num_packets = len(packets)
    if(DEBUG): print("total packets: " + str(num_packets))

    _thread.start_new_thread(receive, (sock,))

    base = 0
    next_to_send = 0

    while base < num_packets:
        mutex.acquire()
        # base is limited to maxseqnum. next_to_send is inc. contin.
        while(next_to_send < base + window_size and next_to_send < num_packets):
            realsno, _ = packet.extract_packet(packets[next_to_send])
            if(DEBUG):print('sending packet no: ' + str(next_to_send) + ' with seq_num: ' + str(realsno))
            udt.send(packets[next_to_send], sock, RECEIVER_ADDR)
            next_to_send += 1

        if not send_timer.running():
            send_timer.start()

        while send_timer.running() and not send_timer.timeout():
            mutex.release()
            # print('Sleeping')
            time.sleep(0.05)
            mutex.acquire()

        if send_timer.timeout():
            if(DEBUG):print('Timeout, resending packets')
            next_to_send = base
            send_timer.stop()
        
        mutex.release()

    udt.send(b'', sock, RECEIVER_ADDR)
    file.close()


def receive(sock):
    global base
    global mutex 

    while True:
        pkt, _ = udt.recv(sock)
        ack, _ = packet.extract_packet(pkt)

        if(DEBUG):print('Received ACK:', ack)
        if(DEBUG):print('Base:', base)
        mutex.acquire()
        base = base + (ack - base%globals.max_seq_num + globals.max_seq_num)%globals.max_seq_num
        if(DEBUG):print('updated Base:', base)
        send_timer.stop()
        mutex.release()


if __name__ == '__main__':
    if len(sys.argv) != 6:
        print('Expected <filename> <packet_size_kb> <window_size> <timeout_interval> <protocol>')
        exit()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(SENDER_ADDR)
    filename = sys.argv[1]
    packet_size = int(sys.argv[2])
    window_size = int(sys.argv[3])
    timeout_interval = float(sys.argv[4])
    protocol = sys.argv[5]

    if(protocol == 'GBN'):
        globals.max_seq_num = window_size + 1  
        send(sock, filename) 
    elif(protocol == 'SW'):
        window_size = 1
        globals.max_seq_num = 2
        send(sock, filename)
    elif(protocol == 'SR'):
        globals.max_seq_num = 2*window_size
        send_sr(sock, filename)
    else:
        print('Invalid protocol')
        exit()


  

    sock.close()
