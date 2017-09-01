#!/bin/bash
ip=$1
port=$2
user=$3
pwd=$4
dbname=$5
filename=$6
tabname=$7
channel_id=$8

if [ $8 == 'SWIFT' ]
then
     echo "swift"
     sed -i "s/\`//g" $filename;
     sed -i '1d' $filename
     sed -i '$d' $filename
     sed -i '$d' $filename      
fi

mysql -h $ip -P $port -u $user -p$pwd << EOF
    use $dbname;
    LOAD DATA LOCAL INFILE "$filename" INTO TABLE $tabname CHARACTER SET utf8 FIELDS TERMINATED BY ',';

EOF
