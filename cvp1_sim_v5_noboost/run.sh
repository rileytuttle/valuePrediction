#!/bin/bash
##########################################################################################
# Spec2006 benchmarks
# 400.perlbench 416.gamess 435.gromacs 445.gobmk 454.calculix 462.libquantum 471.omnetpp
# 483.xalancbmk 401.bzip2 429.mcf 436.cactusADM 447.dealII 456.hmmer 464.h264ref
# 473.astar 403.gcc 433.milc 437.leslie3d 450.soplex 458.sjeng 465.tonto 481.wrf
# 410.bwaves 434.zeusmp 444.namd 453.povray 459.GemsFDTD 470.lbm 482.sphinx3

echo "USAGE: ./run name_of_predictor"
PREDICTOR_NAME=$1
#basepath is where the framework is
BASEPATH=
#keep the trace path as below
TRACEPATH=/local/rtuttle/Downloads/public_kit/
#the BINPATH is where your simulator executable is
BINPATH=$BASEPATH
#all results will be written in results folder as below
RESULTPATH=results


 #benchmarks
 benchmarks1=`echo "tmp433.milc-1-3243-00017.ckpt.23117 tmp444.namd-1-10386-01154.ckpt.23117 tmp444.namd-1-16405-01067.ckpt.23117 tmp447.dealII-1-3537-00701.ckpt.23117 tmp450.soplex-1-1429-02679.ckpt.23117 tmp450.soplex-2-2307-02451.ckpt.23117 tmp453.povray-1-5054-00805.ckpt.23117 tmp470.lbm-1-4367-00022.ckpt.23117 tmp482.sphinx3-1-2085-00711.ckpt.23117 tmp619.lbm_s-1-15769-00002.ckpt.6057 tmp619.lbm_s-1-32833-00001.ckpt.6057 tmp638.imagick_s-1-681180-08594.ckpt.6057 tmp638.imagick_s-1-89598-00006.ckpt.6057 tmp644.nab_s-1-109863-00007.ckpt.6057 tmp644.nab_s-1-8520-00022.ckpt.6057
tmp400.perlbench-1-3666-01705.ckpt.7425 tmp400.perlbench-2-3307-01659.ckpt.7425 tmp400.perlbench-3-4410-00006.ckpt.7425 tmp401.bzip2-1-2489-00391.ckpt.7425 tmp401.bzip2-4-2745-00713.ckpt.7425 tmp401.bzip2-5-3705-00564.ckpt.7425 tmp403.gcc-2-1025-00644.ckpt.7425 tmp403.gcc-2-637-00809.ckpt.7425 tmp403.gcc-8-10-02460.ckpt.7425 tmp429.mcf-1-1569-04578.ckpt.7425 tmp429.mcf-1-2094-01101.ckpt.7425 tmp445.gobmk-1-1598-01623.ckpt.7425 tmp445.gobmk-2-4672-01140.ckpt.7425 tmp456.hmmer-1-355-00002.ckpt.7425 tmp456.hmmer-2-10855-00551.ckpt.7425 tmp464.h264ref-1-3236-00520.ckpt.7425 tmp464.h264ref-3-11896-00015.ckpt.7425 tmp471.omnetpp-1-131-00103.ckpt.7425 tmp471.omnetpp-1-4348-00005.ckpt.7425 tmp473.astar-1-2473-00035.ckpt.7425 tmp473.astar-2-4023-05805.ckpt.7425 tmp483.xalancbmk-1-1305-00274.ckpt.7425 tmp483.xalancbmk-1-7260-00989.ckpt.7425 tmp600.perlbench_s-1-12722-00457.ckpt.31917 tmp600.perlbench_s-3-6765-00006.ckpt.31917 tmp602.gcc_s-1-24023-00002.ckpt.3191 tmp602.gcc_s-2-4627-00014.ckpt.31917 tmp605.mcf_s-1-14562-00011.ckpt.31917 tmp605.mcf_s-1-6685-00283.ckpt.31917 tmp620.omnetpp_s-1-10295-00027.ckpt.31917 tmp620.omnetpp_s-1-6953-08324.ckpt.31917 tmp623.xalancbmk_s-1-11017-00001.ckpt.31917 tmp623.xalancbmk_s-1-7064-00511.ckpt.31917 tmp625.x264_s-1-3166-00533.ckpt.31917 tmp625.x264_s-2-3239-00053.ckpt.31917 tmp625.x264_s-3-6024-02635.ckpt.31917 tmp631.deepsjeng_s-1-15244-00001.ckpt.31917 tmp631.deepsjeng_s-1-7648-00004.ckpt.31917 tmp641.leela_s-1-13960-03019.ckpt.31917 tmp641.leela_s-1-6959-00093.ckpt.31917 tmp657.xz_s-1-18680-02519.ckpt.31917 tmp657.xz_s-1-41612-00006.ckpt.31917 tmp657.xz_s-2-6057-00060.ckpt.31917
tmp18-7-runWorkloadA-2-ycsb-300KDB.ckpt_12_txn720000.ckpt.22297
tmp18-7-runWorkloadA-2-ycsb-300KDB.ckpt_19_txn790000.ckpt.22297
tmp18-7-runWorkloadA-2-ycsb-300KDB.ckpt_5_txn650000.ckpt.22297
tmp18-7-runWorkloadA-2-ycsb-300KDB.ckpt_9_txn690000.ckpt.22297
tmp18-7-runWorkloadB-2-ycsb-300KDB.ckpt_0_txn700000.ckpt.22297
tmp18-7-runWorkloadB-2-ycsb-300KDB.ckpt_12_txn820000.ckpt.22297
tmp18-7-runWorkloadB-2-ycsb-300KDB.ckpt_19_txn890000.ckpt.22297
tmp18-7-runWorkloadB-2-ycsb-300KDB.ckpt_9_txn790000.ckpt.22297
tmp18-7-runWorkloadC-2-ycsb-300KDB.ckpt_12_txn420000.ckpt.22297
tmp18-7-runWorkloadC-2-ycsb-300KDB.ckpt_19_txn490000.ckpt.22297
tmp18-7-runWorkloadC-2-ycsb-300KDB.ckpt_2_txn320000.ckpt.22297
tmp18-7-runWorkloadC-2-ycsb-300KDB.ckpt_9_txn390000.ckpt.22297
tmp18-7-runWorkloadD-2-ycsb-300KDB.ckpt_0_txn600000.ckpt.22297
tmp18-7-runWorkloadD-2-ycsb-300KDB.ckpt_12_txn720000.ckpt.22297
tmp18-7-runWorkloadD-2-ycsb-300KDB.ckpt_19_txn790000.ckpt.22297
tmp18-7-runWorkloadD-2-ycsb-300KDB.ckpt_4_txn640000.ckpt.22297
tmp18-7-runWorkloadE-2-ycsb-300KDB.ckpt_2_txn170000.ckpt.22297
tmp18-7-runWorkloadE-2-ycsb-300KDB.ckpt_8_txn230000.ckpt.22297
tmp18-7-runWorkloadF-2-ycsb-300KDB.ckpt_0_txn300000.ckpt.22297
tmp18-7-runWorkloadF-2-ycsb-300KDB.ckpt_12_txn420000.ckpt.22297
tmp18-7-runWorkloadF-2-ycsb-300KDB.ckpt_19_txn490000.ckpt.22297
tmp18-7-runWorkloadF-2-ycsb-300KDB.ckpt_8_txn380000.ckpt.22297
tmpoltp_delete_2C_10_query150000.ckpt.29938
tmpoltp_delete_2C_40_query450000.ckpt.29938
tmpoltp_insert_2C_10_query150000.ckpt.29938
tmpoltp_insert_2C_40_query450000.ckpt.29938
tmpoltp_point_select_2C_10_query150000.ckpt.29938
tmpoltp_point_select_2C_40_query450000.ckpt.29938
tmpoltp_read_only_2C_10_query150000.ckpt.29938
tmpoltp_read_only_2C_40_query450000.ckpt.29938
tmpoltp_read_write_2C_10_query150000.ckpt.29938
tmpoltp_read_write_2C_40_query450000.ckpt.29938
tmpoltp_update_index_2C_10_query150000.ckpt.29938
tmpoltp_update_index_2C_40_query450000.ckpt.29938
tmpoltp_update_non_index_2C_10_query150000.ckpt.29938
tmpoltp_update_non_index_2C_40_query450000.ckpt.29938
tmpoltp_write_only_2C_10_query150000.ckpt.29938
tmpoltp_write_only_2C_40_query450000.ckpt.29938
tmpRedisclient0_server1_i1_s127.0.0.1_c50_r400000_d8000_ks100000000_p100_tSET-GET-p100_port10001_2C_0_query20000.ckpt.26599
tmpRedisclient0_server1_i1_s127.0.0.1_c50_r400000_d8000_ks100000000_p100_tSET-GET-p100_port10001_2C_17_query190000.ckpt.26599
tmpRedisclient0_server1_i1_s127.0.0.1_c50_r400000_d8000_ks100000000_p100_tSET-GET-p100_port10001_2C_22_query240000.ckpt.26599
tmpRedisclient0_server1_i1_s127.0.0.1_c50_r400000_d8000_ks100000000_p100_tSET-GET-p100_port10001_2C_29_query310000.ckpt.26599
tmpRedisclient0_server1_i1_s127.0.0.1_c50_r400000_d8000_ks100000000_p100_tSET-GET-p100_port10001_2C_34_query360000.ckpt.26599
tmpRedisclient0_server1_i1_s127.0.0.1_c50_r400000_d8000_ks100000000_p1_tSET-GET-p1_port10001_2C_8_query100000.ckpt.26599
tmpRedisclient0_server1_i1_s127.0.0.1_c50_r400000_d8000_ks100000000_p1_tSET-p1_port10001_2C_13_query150000.ckpt.26599
tmpRedisclient0_server1_i1_s127.0.0.1_c50_r400000_d8000_ks100000000_p1_tSET-p1_port10001_2C_18_query200000.ckpt.26599
tmpRedisclient0_server1_i1_s127.0.0.1_c50_r400000_d8000_ks100000000_p1_tSET-p1_port10001_2C_22_query240000.ckpt.26599
tmpRedisclient0_server1_i1_s127.0.0.1_c50_r400000_d8000_ks100000000_p1_tSET-p1_port10001_2C_28_query300000.ckpt.26599
tmpRedisclient0_server1_i1_s127.0.0.1_c50_r400000_d8000_ks100000000_p1_tSET-p1_port10001_2C_31_query330000.ckpt.26599
tmpRedisclient0_server1_i1_s127.0.0.1_c50_r400000_d8000_ks100000000_p1_tSET-p1_port10001_2C_33_query350000.ckpt.26599
tmpRedisclient0_server1_i1_s127.0.0.1_c50_r400000_d8000_ks100000000_p1_tSET-p1_port10001_2C_4_query60000.ckpt.26599
tmpRedisclient0_server1_i1_s127.0.0.1_c50_r400000_d8000_ks100000000_p1_tSET-p1_port10001_2C_7_query90000.ckpt.26599
tmpRedisclient0_server1_i1_s127.0.0.1_c50_r400000_d8000_ks100000000_p1_tSET-p1_port10001_2C_9_query110000.ckpt.26599
tmpselect_random_points_2C_10_query150000.ckpt.29938
tmpselect_random_points_2C_40_query450000.ckpt.29938
tmpselect_random_ranges_2C_10_query150000.ckpt.29938
tmpselect_random_ranges_2C_40_query450000.ckpt.29938
tmpycsb_mongodb_runa_15_txn155100.ckpt.19306
tmpycsb_mongodb_runa_35_txn355100.ckpt.19306
tmpycsb_mongodb_runa_45_txn455100.ckpt.19306
tmpycsb_mongodb_runb_15_txn155100.ckpt.19306
tmpycsb_mongodb_runb_35_txn355100.ckpt.19306
tmpycsb_mongodb_runb_45_txn450100.ckpt.19306
tmpycsb_mongodb_runb_45_txn455100.ckpt.19306
tmpycsb_mongodb_runc_15_txn155100.ckpt.19306
tmpycsb_mongodb_runc_35_txn355100.ckpt.19306
tmpycsb_mongodb_runc_45_txn450100.ckpt.19306
tmpycsb_mongodb_runc_45_txn455100.ckpt.19306
tmpycsb_mongodb_rund_15_txn155100.ckpt.19306
tmpycsb_mongodb_rund_35_txn355100.ckpt.19306
tmpycsb_mongodb_rund_45_txn450100.ckpt.19306
tmpycsb_mongodb_rund_45_txn455100.ckpt.19306
tmpycsb_mongodb_runf_15_txn155100.ckpt.19306
tmpycsb_mongodb_runf_35_txn355100.ckpt.19306
tmpycsb_mongodb_runf_45_txn450100.ckpt.19306
tmpycsb_mongodb_runf_45_txn455100.ckpt.19306"`
 


#benchmarks=`echo "tmp433.milc-1-3243-00017.ckpt.23117"`

 # print the machine name and which traces it will work on
 echo will run traces for ${benchmarks1}


 #Spec2006 benchmarks
 for benchmark in $benchmarks1
 do
     ./cvp -v $TRACEPATH/$benchmark.cvptrace > $RESULTPATH/$benchmark.$PREDICTOR_NAME &
 done
