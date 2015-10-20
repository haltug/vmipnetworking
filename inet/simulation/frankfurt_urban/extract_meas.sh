#!/bin/sh
# "Author: Halis Altug. Version: 1. Date: 16.10.15"

# Configure folder
echo "Running frankfurt_urban"
out=output/1_to_1
N=95

if [ ! -d results ]; then 	
	echo "No results folder exists. Terminating process."
	exit 1 
fi

if [ ! -d output ]; then 
	mkdir output; 
fi
if [ ! -d $out ]; then 
	mkdir $out; 
fi
working_dir=$(pwd)
# echo $working_dir

for sim in TCP_CN_to_MA TCP_MA_to_CN; do
	echo "Extracting $sim"
	if [ ! -d $out/$sim ]; then 
		mkdir $out/$sim; 
	fi
	for idx in $(seq 0 $N); do
		scavetool v -p 'name(sequenceUpdateCa:vector)' -O $working_dir/$out/$sim/ip-$idx.csv -F csv $working_dir/results//$sim-$idx.vec | scavetool v -p 'name(sequenceUpdateDa:vector)' -O $working_dir/$out/$sim/ip_da-$idx.csv -F csv $working_dir/results//$sim-$idx.vec
		scavetool v -p '(module(**MA**) AND name("rcvd seq"))' -O $working_dir/$out/$sim/rcvd-$idx.csv -F csv $working_dir/results//$sim-$idx.vec | scavetool v -p '(module(**MA**) AND name(endToEndDelay:vector))' -O $working_dir/$out/$sim/rtt-$idx.csv -F csv $working_dir/results//$sim-$idx.vec
	done
done

for sim in UDP_CN_to_MA; do
	echo "Extracting $sim"
	if [ ! -d $out/$sim ]; then 
		mkdir $out/$sim; 
	fi
	for idx in $(seq 0 $N); do
		scavetool v -p 'name(sequenceUpdateCa:vector)' -O $working_dir/$out/$sim/ip-$idx.csv -F csv $working_dir/results//$sim-$idx.vec | scavetool v -p 'name(sequenceUpdateDa:vector)' -O $working_dir/$out/$sim/ip_da-$idx.csv -F csv $working_dir/results//$sim-$idx.vec
		scavetool v -p '(module(**MA**) AND name("rcvdPk:vector(packetBytes)"))' -O $working_dir/$out/$sim/rcvd-$idx.csv -F csv $working_dir/results//$sim-$idx.vec | scavetool v -p '(module(**MA**) AND name("endToEndDelay:vector"))' -O $working_dir/$out/$sim/rtt-$idx.csv -F csv $working_dir/results//$sim-$idx.vec
	done
done

for sim in UDP_MA_to_CN; do
	echo "Extracting $sim"
	if [ ! -d $out/$sim ]; then 
		mkdir $out/$sim; 
	fi
	for idx in $(seq 0 $N); do
		scavetool v -p 'name(sequenceUpdateCa:vector)' -O $working_dir/$out/$sim/ip-$idx.csv -F csv $working_dir/results//$sim-$idx.vec | scavetool v -p 'name(sequenceUpdateDa:vector)' -O $working_dir/$out/$sim/ip_da-$idx.csv -F csv $working_dir/results//$sim-$idx.vec
		scavetool v -p '(module(**MA**) AND name("rcvdPk:vector(packetBytes)"))' -O $working_dir/$out/$sim/rcvd-$idx.csv -F csv $working_dir/results//$sim-$idx.vec | scavetool v -p '(module(**MA**) AND name("rcvdPkLifetime:vector"))' -O $working_dir/$out/$sim/rtt-$idx.csv -F csv $working_dir/results//$sim-$idx.vec
	done
done

echo "Done!"
