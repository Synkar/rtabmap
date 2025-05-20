#!/bin/bash

if [ $# -lt 3 ]
then
  echo "No arguments supplied. They should be 3: the detector number type (0=SURF 1=SIFT 2=ORB 3=FAST/FREAK 4=FAST/BRIEF 5=GFTT/FREAK 6=GFTT/BRIEF 7=BRISK 8=GFTT/ORB 9=KAZE 10=ORB-OCTREE 11=SuperPoint), the input directory (original maps) and the output data directory (where reprocessed map databases will be saved)."
  exit
fi
TYPE=$1
INPUT=$2
OUTPUT=$3


source rtabmap_latest.bash

DATABASES=( 'dabi_09_58_floor0-5.2.1-2025-05-15.db' 'dabi_12_25_floor0-5.2.1-2025-05-16.db' 'dabi_15_12_floor0-5.2.1-2025-05-16.db' 'dabi_17_11_floor0-5.2.1-2025-05-16.db' 'merged_1234.db' 'merged_13.db' 'merged_24.db' 'merged_14.db')

LOCALIZATION_DATABASES="$INPUT/dabi_9_45_floor0-5.2.1-2025-04-29.db;$INPUT/dabi_11_43_floor0-5.2.1-2025-04-29.db;$INPUT/dabi_15_15_floor0-5.2.1-2025-04-30.db;$INPUT/dabi_17_22_floor0-5.2.1-2025-05-07.db"

[ ! -d "$OUTPUT/$TYPE/loc" ] && mkdir $OUTPUT/$TYPE/loc

echo $PARAMS
for db in "${DATABASES[@]}"
do
  rtabmap-reprocess -loc_null --Mem/IncrementalMemory false --RGBD/ProximityBySpace true --RGBD/ProximityMaxPaths 3 --Mem/LocalizationDataSaved true --Mem/BinDataKept true --RGBD/SavedLocalizationIgnored true --Kp/IncrementalFlann true --Vis/MinInliers 30 --Rtabmap/PublishRAMUsage true --RGBD/ProximityOdomGuess false --uwarn "$OUTPUT/$TYPE/$db;$LOCALIZATION_DATABASES" $OUTPUT/$TYPE/loc/loc_$db
done


