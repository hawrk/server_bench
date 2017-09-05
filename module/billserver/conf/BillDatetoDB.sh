#!/bin/bash
ip=$1
port=$2
user=$3
pwd=$4
dbname=$5
filename=$6
tabname=$7


mysql -h $ip -P $port -u $user -p$pwd << EOF
    use $dbname;
    LOAD DATA LOCAL INFILE "$filename" INTO TABLE $tabname CHARACTER SET utf8 FIELDS TERMINATED BY ',';

EOF
