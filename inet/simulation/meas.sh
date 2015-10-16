#!/bin/sh
echo 'Running Measurement Tool'

# ----------------------------------------------------------------------------------------------------------------
# TEST 1
cd frankfurt_urban
bash extract_meas.sh &
cd ..
cd frankfurt_urban_mipv6
bash extract_meas.sh &
cd ..
cd frankfurt_urban_multi_radio
bash extract_meas.sh 
cd ..
cd frankfurt_urban_n_vehicles
bash extract_meas.sh &
cd ..
cd frankfurt_urban_n_vehicles_mipv6
bash extract_meas.sh &
cd ..
cd frankfurt_urban_n_vehicles_multi_radio
bash extract_meas.sh
# cd ..
# cd frankfurt_urban_n_nodes
# cd ..
# cd frankfurt_urban_n_nodes_mipv6
# cd ..
# cd frankfurt_urban_n_nodes_multi_radio