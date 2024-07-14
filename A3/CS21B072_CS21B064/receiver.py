# receiver.py
import packet
import socket
import sys
import time 
import udt
import globals

DEBUG = 0
RECEIVER_ADDR = ('localhost', 8080)

def receive(sock, filename):
    try:
        file = open(filename, 'wb')
    except IOError:
        print('Unable to open', filename)
        return
    
    expected_num = 0
    start_time = time.time()
    while True:
        pkt, addr = udt.recv(sock)
        if not pkt:
            break
        seq_num, data = packet.extract_packet(pkt)
        if(DEBUG):print('Got packet: ', seq_num)

        if seq_num == expected_num:
            if(DEBUG):print('Got expected packet')
            if(DEBUG):print('Sending ACK:', (expected_num+1)%globals.max_seq_num)
            ack_pkt = packet.make_packet((expected_num+1)%globals.max_seq_num, b'')
            expected_num = (expected_num + 1) % globals.max_seq_num
            udt.send(ack_pkt, sock, addr)
            file.write(data)
        else:
            # discard the packet
            if(DEBUG):print('expected packet:', expected_num)
            if(DEBUG):print('Discarding packet:', seq_num)

    end_time = time.time()
    print('Download time: ', end_time - start_time)
    file.close()

def receive_sr(sock, filename):
    try:
        file = open(filename, 'wb')
    except IOError:
        print('Unable to open', filename)
        return
    
    # maintain a buffer of packets holding out of order packets - it's a set, always sorted
    buffer = []
    receiver_base = 0
    # receiver window - 0, 1, 2, 3, ... , window_size-1

    start_time = time.time()
    while True:
        pkt, addr = udt.recv(sock)
        if not pkt:
            break
        seq_num, data = packet.extract_packet(pkt)
        if(DEBUG):print('Got packet: ', seq_num)
        if seq_num == receiver_base:
            # check elements in buffer, and find cumulatively received packets
            cum_received = seq_num  
            # iterate over buffer, and find cumulatively received packets
            front_to_remove = 0
            file.write(data)
            for i in range(len(buffer)):
                if buffer[i][0] == (cum_received + 1)%globals.max_seq_num:
                    cum_received  = (cum_received + 1)%globals.max_seq_num
                    file.write(buffer[i][1])
                    front_to_remove += 1
                else:
                    break
            
            # remove the front_to_remove elements from the buffer
            buffer = buffer[front_to_remove:]

            print("sending ack - on receiver side" , cum_received)
            ack_pkt = packet.make_packet((cum_received+1)%globals.max_seq_num, b'')
            udt.send(ack_pkt, sock, addr)
            receiver_base = (cum_received+1)%globals.max_seq_num

        elif (seq_num - receiver_base + globals.max_seq_num)%globals.max_seq_num < globals.window_size:
            buffer.append((seq_num, data))
            # keep it sorted by key
            # still send the ack for the receiver_base
            ack_pkt = packet.make_packet((receiver_base)%globals.max_seq_num, b'')
            udt.send(ack_pkt, sock, addr)
        else:
            # discard the packet
            if(DEBUG):print('expected packet:', receiver_base, 'to and so on')
            if(DEBUG):print('Discarding packet:', seq_num)
    end_time = time.time()
    print('Download time: ', end_time - start_time)
    file.close()


if __name__ == '__main__':
    if len(sys.argv) != 4:
        print('Expected filename as command line argument')
        exit()

        
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(RECEIVER_ADDR) 
    filename = sys.argv[1]
    globals.window_size = int(sys.argv[2])
    protocol = sys.argv[3]

    if(protocol == 'GBN'):
        globals.max_seq_num = globals.window_size + 1
        receive(sock, filename)
    elif(protocol == 'SW'):
        globals.window_size = 1
        globals.max_seq_num = 2
        receive(sock, filename)
    elif(protocol == 'SR'):
        globals.max_seq_num = 2*globals.window_size
        receive_sr(sock, filename)


    sock.close()