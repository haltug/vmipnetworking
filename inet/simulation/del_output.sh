#!/bin/sh
echo 'Deleting output folder'

# ----------------------------------------------------------------------------------------------------------------
# TEST 1
cd frankfurt_urban
rm -r output
cd ..
cd frankfurt_urban_mipv6
rm -r output
cd ..
cd frankfurt_urban_multi_radio
rm -r output
cd ..
cd frankfurt_urban_n_vehicles
rm -r output
cd ..
cd frankfurt_urban_n_vehicles_mipv6
rm -r output
cd ..
cd frankfurt_urban_n_vehicles_multi_radio
rm -r output
# cd ..
# cd frankfurt_urban_n_nodes
# cd ..
# cd frankfurt_urban_n_nodes_mipv6
# cd ..
# cd frankfurt_urban_n_nodes_multi_radio