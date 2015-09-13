#!/bin/sh

cd /Users/halis/vmipnetworking/inet/simulation
echo 'Running Omnet Simulation'

# ----------------------------------------------------------------------------------------------------------------
# TEST 1
echo '######################################################################################################################'
echo ' frankfurt_urban'
echo '######################################################################################################################'
cd frankfurt_urban
echo '================================== UDP MA-to-CN ========================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..19
echo '================================== UDP MA-to-CN 2 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 0..19
echo '================================== UDP MA-to-CN 3 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 0..19
echo '================================== UDP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..19
echo '================================== TCP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..19
echo '================================== TCP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..19

# ----------------------------------------------------------------------------------------------------------------
# TEST 2
echo '######################################################################################################################'
echo ' frankfurt_urban_n_nodes'
echo '######################################################################################################################'
cd ..
cd frankfurt_urban_n_nodes
echo '================================== UDP MA-to-CN ========================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..19
echo '================================== UDP MA-to-CN 2 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 0..19
echo '================================== UDP MA-to-CN 3 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 0..19
echo '================================== UDP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..19
# echo '================================== TCP CN-to-MA ========================================='
# opp_runall -j1 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..19
echo '================================== TCP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..19


# ----------------------------------------------------------------------------------------------------------------
# TEST 3
echo '######################################################################################################################'
echo ' frankfurt_urban_n_vehicles'
echo '######################################################################################################################'
cd ..
cd frankfurt_urban_n_vehicles
# echo '================================== UDP MA-to-CN ========================================='
# opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..19
echo '================================== UDP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..19
echo '================================== UDP MA-to-CN 2 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 0..19
echo '================================== UDP MA-to-CN 3 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 0..19
# echo '======================================================================================================================'
# echo '================================== TCP CN-to-MA ========================================='
# opp_runall -j1 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..19
echo '================================== TCP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..19

# ----------------------------------------------------------------------------------------------------------------
# TEST 4
echo '######################################################################################################################'
echo ' frankfurt_urban'
echo '######################################################################################################################'
cd ..
cd frankfurt_urban
echo '================================== UDP MA-to-CN ========================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..19
echo '================================== UDP MA-to-CN 2 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 0..19
echo '================================== UDP MA-to-CN 3 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 0..19
echo '================================== UDP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..19
echo '================================== TCP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..19
echo '================================== TCP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..19

# ----------------------------------------------------------------------------------------------------------------
# TEST 5
echo '######################################################################################################################'
echo ' frankfurt_urban_n_nodes'
echo '######################################################################################################################'
cd ..
cd frankfurt_urban_n_nodes
echo '================================== UDP MA-to-CN ========================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..19
echo '================================== UDP MA-to-CN 2 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 0..19
echo '================================== UDP MA-to-CN 3 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 0..19
echo '================================== UDP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..19
# echo '================================== TCP CN-to-MA ========================================='
# opp_runall -j1 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..19
echo '================================== TCP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..19


# ----------------------------------------------------------------------------------------------------------------
# TEST 6
echo '######################################################################################################################'
echo ' frankfurt_urban_n_vehicles'
echo '######################################################################################################################'
cd ..
cd frankfurt_urban_n_vehicles
# echo '================================== UDP MA-to-CN ========================================='
# opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..19
echo '================================== UDP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..19
echo '================================== UDP MA-to-CN 2 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 0..19
echo '================================== UDP MA-to-CN 3 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 0..19
# echo '======================================================================================================================'
# echo '================================== TCP CN-to-MA ========================================='
# opp_runall -j1 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..19
echo '================================== TCP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..19

# ----------------------------------------------------------------------------------------------------------------
# TEST 7
echo '######################################################################################################################'
echo ' frankfurt_urban_mipv6'
echo '######################################################################################################################'
cd ..
cd frankfurt_urban_mipv6
echo '================================== UDP MA-to-CN ========================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..19
echo '================================== UDP MA-to-CN 2 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 0..19
echo '================================== UDP MA-to-CN 3 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 0..19
echo '================================== UDP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..19
echo '================================== TCP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..19
echo '================================== TCP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..19


# ----------------------------------------------------------------------------------------------------------------
# TEST 8
echo '######################################################################################################################'
echo ' frankfurt_urban_n_nodes_mipv6'
echo '######################################################################################################################'
cd ..
cd frankfurt_urban_n_nodes_mipv6
# echo '================================== UDP MA-to-CN ========================================='
# opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..19
echo '================================== UDP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..19
echo '================================== UDP MA-to-CN 2 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 0..19
echo '================================== UDP MA-to-CN 3 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 0..19
# echo '======================================================================================================================'
# echo '================================== TCP CN-to-MA ========================================='
# opp_runall -j1 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..19
echo '================================== TCP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..19


# ----------------------------------------------------------------------------------------------------------------
# TEST 9
echo '######################################################################################################################'
echo ' frankfurt_urban_n_vehicles_mipv6'
echo '######################################################################################################################'
cd ..
cd frankfurt_urban_n_vehicles_mipv6
echo '================================== UDP MA-to-CN ========================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..19
echo '================================== UDP MA-to-CN 2 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 0..19
echo '================================== UDP MA-to-CN 3 ======================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 0..19
echo '================================== UDP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..19
# echo '================================== TCP CN-to-MA ========================================='
# opp_runall -j1 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..19
echo '================================== TCP CN-to-MA ========================================='
opp_runall -j1 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..19
