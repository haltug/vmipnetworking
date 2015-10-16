#!/bin/sh
# "Author: Halis Altug. Version: 1. Date: 16.10.15"

# Configure folder
echo "Running frankfurt_urban_n_vehicles_multi_radio"
out=output/n_to_1
N=31
CAR=9

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
		if [ ! -d $out/$sim/$idx ]; then 
			mkdir $out/$sim/$idx; 
		fi
		for car in $(seq 0 $CAR); do
			scavetool v -p 'module(**.MA['$car'].**) AND name(sequenceUpdateCa:vector)' -O $working_dir/$out/$sim/$idx/ip-$car.csv -F csv $working_dir/results/$sim-$idx.vec | scavetool v -p 'module(**.MA['$car'].**) AND name(sequenceUpdateDa:vector)' -O $working_dir/$out/$sim/$idx/ip_da-$car.csv -F csv $working_dir/results/$sim-$idx.vec
			scavetool v -p 'module(**.MA['$car'].**) AND name("rcvd seq")' -O $working_dir/$out/$sim/$idx/rcvd-$car.csv -F csv $working_dir/results/$sim-$idx.vec | scavetool v -p 'module(**.MA['$car'].**) AND name(endToEndDelay:vector)' -O $working_dir/$out/$sim/$idx/rtt-$car.csv -F csv $working_dir/results/$sim-$idx.vec
		done
	done
done

for sim in UDP_CN_to_MA; do
	echo "Extracting $sim"
	if [ ! -d $out/$sim ]; then 
		mkdir $out/$sim; 
	fi
	for idx in $(seq 0 $N); do
		if [ ! -d $out/$sim/$idx ]; then 
			mkdir $out/$sim/$idx; 
		fi
		for car in $(seq 0 $CAR); do
			scavetool v -p 'module(**.MA['$car'].**) AND name(sequenceUpdateCa:vector)' -O $working_dir/$out/$sim/$idx/ip-$car.csv -F csv $working_dir/results/$sim-$idx.vec | scavetool v -p 'module(**.MA['$car'].**) AND name(sequenceUpdateDa:vector)' -O $working_dir/$out/$sim/$idx/ip_da-$car.csv -F csv $working_dir/results/$sim-$idx.vec
			scavetool v -p 'module(**.MA['$car'].**) AND name("rcvdPk:vector(packetBytes)")' -O $working_dir/$out/$sim/$idx/rcvd-$car.csv -F csv $working_dir/results/$sim-$idx.vec | scavetool v -p 'module(**.MA['$car'].**) AND name(endToEndDelay:vector)' -O $working_dir/$out/$sim/$idx/rtt-$car.csv -F csv $working_dir/results/$sim-$idx.vec
		done
	done
done

for sim in UDP_MA_to_CN; do
	echo "Extracting $sim"
	if [ ! -d $out/$sim ]; then 
		mkdir $out/$sim; 
	fi
	for idx in $(seq 0 $N); do
		if [ ! -d $out/$sim/$idx ]; then 
			mkdir $out/$sim/$idx; 
		fi
		for car in $(seq 0 $CAR); do
			scavetool v -p 'module(**.MA['$car'].**) AND name(sequenceUpdateCa:vector)' -O $working_dir/$out/$sim/$idx/ip-$car.csv -F csv $working_dir/results/$sim-$idx.vec | scavetool v -p 'module(**.MA['$car'].**) AND name(sequenceUpdateDa:vector)' -O $working_dir/$out/$sim/$idx/ip_da-$car.csv -F csv $working_dir/results/$sim-$idx.vec
			scavetool v -p 'module(**.MA['$car'].**) AND name("rcvdPk:vector(packetBytes)")' -O $working_dir/$out/$sim/$idx/rcvd-$car.csv -F csv $working_dir/results/$sim-$idx.vec | scavetool v -p 'module(**.MA['$car'].**) AND name(rcvdPkLifetime:vector)' -O $working_dir/$out/$sim/$idx/rtt-$car.csv -F csv $working_dir/results/$sim-$idx.vec
		done
	done
done

echo "Done!"