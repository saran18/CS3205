#!/bin/bash

protocol=('SR' 'GBN' 'SW')
packet_loss_values=(0.1 0.5 1 1.5 2 5) # Percentage/probability of packet loss
latency_values=(50 100 150 200 250 500) # Latency in ms
latency_deviation=10
bandwidth=8000kbit # In KB/s
packet_size=25600 # In KB
window_size=7
output_file="dest.jpg"
input_file='loco.jpg' 
timing_file="timing.txt"
interface='lo'
# Iterate over the list using a for loop
# sudo tc qdisc add dev $interface root netem rate $bandwidth
for curr_protocol in "${protocol[@]}"
do
    for packet_loss in "${packet_loss_values[@]}"
    do
        for latency in "${latency_values[@]}"
        do 
            # sudo tc qdisc change dev $interface root netem delay $latency $latency_deviation
            echo $latency $packet_loss $curr_protocol
            python receiver.py $output_file $window_size $protocol
            difference=$(diff "$input_file" "$output_file")
            if [ -n "$difference" ]; then
                echo "Files differ."
                echo "$difference"
                exit 1
            fi

            rm $output_file
        done
    done
done
# sudo tc qdisc del dev $interface root netem