1) Run the Receiver:

Open a terminal window and navigate to the directory.
Run the receiver script by executing the following command:

python receiver.py <filename> <window_size> <protocol>

Replace <filename> with the name of the file to be received, <window_size> with the desired window size, and <protocol> with the desired protocol (either 'GBN', 'SW', or 'SR').

2) Run the Sender:

Open another terminal window and navigate to the directory.
Run the sender script by executing the following command:

python udp_server.py <filename> <packet_size_kb> <window_size> <timeout_interval> <protocol>

Replace <filename> with the name of the file to be sent, <packet_size_kb> with the packet size in bytes (misnomer), <window_size> with the desired window size, <timeout_interval> with the timeout interval, and <protocol> with the desired protocol (either 'GBN', 'SW', or 'SR').


3) Make sure to modify RECEIVER_ADDR, SENDER_ADDR accordingly.

4) Check your receiver terminal for download time, and check the destination image for the result. 


GENERATING PLOTS

1) We ran the 'run_tests_server.sh' and 'run_receivers.sh' scripts on two different terminals and observed the times.