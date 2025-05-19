#!/bin/bash

if [ $# -lt 2 ]
then
  echo "No arguments supplied. It should be the detector number type (0=SURF 1=SIFT 2=ORB 3=FAST/FREAK 4=FAST/BRIEF 5=GFTT/FREAK 6=GFTT/BRIEF 7=BRISK 8=GFTT/ORB 9=KAZE 10=ORB-OCTREE 11=SuperPoint) and the data directory (where map databases have been reprocessed)."
  exit
fi
TYPE=$1
DATA=$2

MIN_INLIERS=20 #20 40 60 80

source rtabmap_latest.bash

DATABASES="$DATA/$TYPE/dabi_09_58_floor0-5.2.1-2025-05-15.db;$DATA/$TYPE/dabi_12_25_floor0-5.2.1-2025-05-16.db;$DATA/$TYPE/dabi_15_12_floor0-5.2.1-2025-05-16.db;$DATA/$TYPE/dabi_17_11_floor0-5.2.1-2025-05-16.db"

# To compute "Ground truth"
rtabmap-reprocess --uwarn "$DATABASES" $DATA/$TYPE/merged_1234.db

rtabmap-reprocess --uwarn "$DATA/$TYPE/dabi_09_58_floor0-5.2.1-2025-05-15.db;$DATA/$TYPE/dabi_17_11_floor0-5.2.1-2025-05-16.db" $DATA/$TYPE/merged_14.db

rtabmap-reprocess --uwarn "$DATA/$TYPE/dabi_09_58_floor0-5.2.1-2025-05-15.db;$DATA/$TYPE/dabi_15_12_floor0-5.2.1-2025-05-16.db" $DATA/$TYPE/merged_13.db

rtabmap-reprocess --uwarn "$DATA/$TYPE/dabi_12_25_floor0-5.2.1-2025-05-16.db;$DATA/$TYPE/dabi_17_11_floor0-5.2.1-2025-05-16.db" $DATA/$TYPE/merged_24.db
