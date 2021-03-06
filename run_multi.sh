#!/bin/sh

cd /Users/halis/vmipnetworking/inet/simulation
echo 'Running Omnet Simulation'


# ----------------------------------------------------------------------------------------------------------------
# TEST 4
echo '##################################################################################################################################################'
echo ' frankfurt_urban_multi_radio'
echo '##################################################################################################################################################'
cd frankfurt_urban_multi_radio
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
echo '##################################################################################################################################################'
echo ' frankfurt_urban_n_nodes_multi_radio'
echo '##################################################################################################################################################'
cd ..
cd frankfurt_urban_n_nodes_multi_radio
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
echo '##################################################################################################################################################'
echo ' frankfurt_urban_n_vehicles_multi_radio'
echo '##################################################################################################################################################'
cd ..
cd frankfurt_urban_n_vehicles_multi_radio
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
