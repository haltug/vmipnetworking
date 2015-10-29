#!/bin/sh

cd /home/halis/vmipnetworking/inet/simulation
echo 'Running Omnet Simulation'

# ----------------------------------------------------------------------------------------------------------------
# TEST 1
echo '##################################################################################################################################################'
echo ' frankfurt_urban'
echo '##################################################################################################################################################'
cd frankfurt_urban
# echo '================================== UDP MA-to-CN =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 24..31
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 32..39
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 40..47
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 48..55
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 56..63
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 64..71
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 72..79
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 80..87
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 88..95

# echo '================================== UDP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 24..31
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 32..39
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 40..47
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 48..55
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 56..63
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 64..71
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 72..79
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 80..87
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 88..95

# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 24..31
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 32..39
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 40..47
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 48..55
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 56..63
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 64..71
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 72..79
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 80..87
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 88..95
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 24..31
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 32..39
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 40..47
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 48..55
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 56..63
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 64..71
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 72..79
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 80..87
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 88..95

# echo '================================== UDP MA-to-CN2 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 0..7
# echo '================================== UDP MA-to-CN2 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 8..15
# echo '================================== UDP MA-to-CN2 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 16..23
# echo '================================== UDP MA-to-CN3 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 0..7
# echo '================================== UDP MA-to-CN3 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 8..15
# echo '================================== UDP MA-to-CN3 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 16..23


# # ----------------------------------------------------------------------------------------------------------------
# # TEST 2
# echo '##################################################################################################################################################'
# echo ' frankfurt_urban_n_nodes'
# echo '##################################################################################################################################################'
# cd ..
# cd frankfurt_urban_n_nodes
# echo '================================== UDP MA-to-CN =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..7
# echo '================================== UDP MA-to-CN =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 8..15
# echo '================================== UDP MA-to-CN =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 16..23
# echo '================================== UDP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..7
# echo '================================== UDP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 8..15
# echo '================================== UDP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 16..23
# echo '================================== UDP MA-to-CN2 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 0..7
# echo '================================== UDP MA-to-CN2 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 8..15
# echo '================================== UDP MA-to-CN2 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 16..23
# echo '================================== UDP MA-to-CN3 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 0..7
# echo '================================== UDP MA-to-CN3 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 8..15
# echo '================================== UDP MA-to-CN3 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 16..23
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..7
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 8..15
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 16..23
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..7
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 8..15
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 16..23



# # ----------------------------------------------------------------------------------------------------------------
# # TEST 3
# echo '##################################################################################################################################################'
# echo ' frankfurt_urban_n_vehicles'
# echo '##################################################################################################################################################'
# cd ..
# cd frankfurt_urban_n_vehicles
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 24..31
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 32..39
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 40..47
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 48..55
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 56..63
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 64..71
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 72..79
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 80..87
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 88..95
# echo '================================== UDP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 24..31
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 32..39
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 40..47
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 48..55
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 56..63
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 64..71
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 72..79
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 80..87
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 88..95
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 24..31
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 32..39
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 40..47
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 48..55
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 56..63
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 64..71
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 72..79
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 80..87
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 88..95
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 24..31
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 32..39
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 40..47
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 48..55
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 56..63
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 64..71
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 72..79
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 80..87
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 88..95



# ----------------------------------------------------------------------------------------------------------------
# TEST 4
echo '##################################################################################################################################################'
echo ' frankfurt_urban_multi_radio'
echo '##################################################################################################################################################'
cd ..
cd frankfurt_urban_multi_radio
echo '================================== UDP MA-to-CN =================================================================================='
opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..7
opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 8..15
opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 16..23
opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 24..31
opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 32..39
opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 40..47
opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 48..55
opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 56..63
opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 64..71
opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 72..79
opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 80..87
opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 88..95

echo '================================== UDP CN-to-MA =================================================================================='
opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..7
opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 8..15
opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 16..23
opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 24..31
opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 32..39
opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 40..47
opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 48..55
opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 56..63
opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 64..71
opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 72..79
opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 80..87
opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 88..95

echo '================================== TCP CN-to-MA =================================================================================='
opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..7
opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 8..15
opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 16..23
opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 24..31
opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 32..39
opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 40..47
opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 48..55
opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 56..63
opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 64..71
opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 72..79
opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 80..87
opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 88..95
echo '================================== TCP CN-to-MA =================================================================================='
opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..7
opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 8..15
opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 16..23
opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 24..31
opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 32..39
opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 40..47
opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 48..55
opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 56..63
opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 64..71
opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 72..79
opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 80..87
opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 88..95


# # ----------------------------------------------------------------------------------------------------------------
# # TEST 5
# echo '##################################################################################################################################################'
# echo ' frankfurt_urban_n_nodes_multi_radio'
# echo '##################################################################################################################################################'
# cd ..
# cd frankfurt_urban_n_nodes_multi_radio
# echo '================================== UDP MA-to-CN =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..7
# echo '================================== UDP MA-to-CN =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 8..15
# echo '================================== UDP MA-to-CN =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 16..23
# echo '================================== UDP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..7
# echo '================================== UDP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 8..15
# echo '================================== UDP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 16..23
# echo '================================== UDP MA-to-CN2 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 0..7
# echo '================================== UDP MA-to-CN2 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 8..15
# echo '================================== UDP MA-to-CN2 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 16..23
# echo '================================== UDP MA-to-CN3 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 0..7
# echo '================================== UDP MA-to-CN3 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 8..15
# echo '================================== UDP MA-to-CN3 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 16..23
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..7
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 8..15
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 16..23
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..7
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 8..15
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 16..23



# ----------------------------------------------------------------------------------------------------------------
# TEST 6
# echo '##################################################################################################################################################'
# echo ' frankfurt_urban_n_vehicles_multi_radio'
# echo '##################################################################################################################################################'
# cd ..
# cd frankfurt_urban_n_vehicles_multi_radio
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 24..31
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 32..39
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 40..47
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 48..55
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 56..63
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 64..71
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 72..79
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 80..87
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 88..95
# echo '================================== UDP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 24..31
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 32..39
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 40..47
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 48..55
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 56..63
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 64..71
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 72..79
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 80..87
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 88..95
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 24..31
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 32..39
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 40..47
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 48..55
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 56..63
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 64..71
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 72..79
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 80..87
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 88..95
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 24..31
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 32..39
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 40..47
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 48..55
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 56..63
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 64..71
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 72..79
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 80..87
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 88..95



# ----------------------------------------------------------------------------------------------------------------
# TEST 7
# echo '##################################################################################################################################################'
# echo ' frankfurt_urban_mipv6'
# echo '##################################################################################################################################################'
# cd ..
# cd frankfurt_urban_mipv6
# echo '================================== UDP MA-to-CN =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 24..31
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 32..39
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 40..47
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 48..55
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 56..63
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 64..71
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 72..79
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 80..87
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 88..95

# echo '================================== UDP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 24..31
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 32..39
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 40..47
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 48..55
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 56..63
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 64..71
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 72..79
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 80..87
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 88..95

# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 24..31
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 32..39
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 40..47
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 48..55
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 56..63
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 64..71
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 72..79
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 80..87
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 88..95
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 24..31
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 32..39
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 40..47
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 48..55
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 56..63
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 64..71
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 72..79
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 80..87
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 88..95



# # ----------------------------------------------------------------------------------------------------------------
# # TEST 8
# echo '##################################################################################################################################################'
# echo ' frankfurt_urban_n_nodes_mipv6'
# echo '##################################################################################################################################################'
# cd ..
# cd frankfurt_urban_n_nodes_mipv6
# echo '================================== UDP MA-to-CN =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..7
# echo '================================== UDP MA-to-CN =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 8..15
# echo '================================== UDP MA-to-CN =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 16..23
# echo '================================== UDP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..7
# echo '================================== UDP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 8..15
# echo '================================== UDP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 16..23
# echo '================================== UDP MA-to-CN2 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 0..7
# echo '================================== UDP MA-to-CN2 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 8..15
# echo '================================== UDP MA-to-CN2 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN2 -r 16..23
# echo '================================== UDP MA-to-CN3 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 0..7
# echo '================================== UDP MA-to-CN3 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 8..15
# echo '================================== UDP MA-to-CN3 ================================================================================'
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN3 -r 16..23
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..7
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 8..15
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 16..23
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..7
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 8..15
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 16..23



# ----------------------------------------------------------------------------------------------------------------
# TEST 9
# echo '##################################################################################################################################################'
# echo ' frankfurt_urban_n_vehicles_mipv6'
# echo '##################################################################################################################################################'
# cd ..
# cd frankfurt_urban_n_vehicles_mipv6
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 24..31
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 32..39
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 40..47
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 48..55
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 56..63
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 64..71
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 72..79
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 80..87
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_MA_to_CN -r 88..95
# echo '================================== UDP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 24..31
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 32..39
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 40..47
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 48..55
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 56..63
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 64..71
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 72..79
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 80..87
# # opp_runall -j8 opp_run -u Cmdenv -c UDP_CN_to_MA -r 88..95
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 24..31
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 32..39
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 40..47
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 48..55
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 56..63
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 64..71
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 72..79
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 80..87
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_CN_to_MA -r 88..95
# echo '================================== TCP CN-to-MA =================================================================================='
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 0..7
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 8..15
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 16..23
# opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 24..31
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 32..39
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 40..47
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 48..55
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 56..63
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 64..71
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 72..79
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 80..87
# # opp_runall -j8 opp_run -u Cmdenv -c TCP_MA_to_CN -r 88..95

cd ..
bash meas.sh