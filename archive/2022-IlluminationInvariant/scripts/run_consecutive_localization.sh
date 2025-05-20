#!/bin/bash

if [ $# -lt 2 ]
then
  echo "No arguments supplied. It should be the detector number type (0=SURF 1=SIFT 2=ORB 3=FAST/FREAK 4=FAST/BRIEF 5=GFTT/FREAK 6=GFTT/BRIEF 7=BRISK 8=GFTT/ORB 9=KAZE 10=ORB-OCTREE 11=SuperPoint) and the data directory (where map databases have been reprocessed)."
  exit
fi
TYPE=$1
DATA=$2

source rtabmap_latest.bash

SOURCE=('dabi_09_58_floor0-5.2.1-2025-05-15.db' 'dabi_12_25_floor0-5.2.1-2025-05-16.db' 'dabi_15_12_floor0-5.2.1-2025-05-16.db')
TARGETS=($DATA/$TYPE'/dabi_12_25_floor0-5.2.1-2025-05-16.db;'$DATA/$TYPE'/dabi_15_12_floor0-5.2.1-2025-05-16.db;'$DATA/$TYPE'/dabi_17_11_floor0-5.2.1-2025-05-16.db' $DATA/$TYPE'/dabi_15_12_floor0-5.2.1-2025-05-16.db;'$DATA/$TYPE'/dabi_17_11_floor0-5.2.1-2025-05-16.db;' $DATA/$TYPE'/dabi_17_11_floor0-5.2.1-2025-05-16.db' )


[ ! -d "$DATA/$TYPE/consecutive_loc" ] && mkdir $DATA/$TYPE/consecutive_loc

for i in ${!SOURCE[@]}
do
    db=${SOURCE[$i]}
    loc_dbs=${TARGETS[$i]}
    rtabmap-reprocess --Mem/IncrementalMemory false --RGBD/ProximityBySpace true --RGBD/ProximityMaxPaths 1 --Mem/LocalizationDataSaved true --Mem/BinDataKept false --RGBD/SavedLocalizationIgnored true --Kp/IncrementalFlann false --Vis/MinInliers 20 --Rtabmap/PublishRAMUsage true --RGBD/ProximityOdomGuess false --uwarn "$DATA/$TYPE/$db;$loc_dbs" $DATA/$TYPE/consecutive_loc/loc_$db
done

