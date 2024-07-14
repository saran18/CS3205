#!/bin/bash


# Define the list
packet_loss_values=(0.1% 0.5% 1% 1.5% 2% 5%) # Percentage/probability of packet loss
latency_values=(50ms 100ms 150ms 200ms 250ms 500ms) # Latency in ms
protocol=('SR' 'GBN' 'SW')
latency_deviation=10
bandwidth=8000kbit # In KB/s -> Kbits
packet_size=25600 # In KB -> bytes
window_size=8
RTO=1 # In s
input_file='loco.jpg' 
interface='wlo1'

if ! sudo tc qdisc show dev $interface | grep -q "netem"; then
    sudo tc qdisc add dev $interface root netem rate $bandwidth
fi
# Iterate over the list using a for loop
# sudo tc qdisc add dev $interface root netem rate $bandwidth
for curr_protocol in "${protocol[@]}"
do
    for packet_loss in "${packet_loss_values[@]}"
    do
        for latency in "${latency_values[@]}"
        do 
            sudo tc qdisc change dev $interface root netem delay $latency $latency_deviation loss $packet_loss rate $bandwidth
            python udp_server.py $input_file $packet_size $window_size $RTO $curr_protocol

        done
    done
done
sudo tc qdisc del dev $interface root netem