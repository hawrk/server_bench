#!/bin/bash
filename=$1
if [ $2 == 'wx' ]
then
	echo "wx"
	sed -i "s/\`//g" $filename;
	sed -i '1d' $filename
    sed -i '$d' $filename
    sed -i '$d' $filename
    sed -i "s/^/$3,/g" $filename
elif [ $2 == 'ali' ]
then
     echo 'ali'
     sed -i '1d' $filename
     sed -i '1d' $filename
     sed -i '1d' $filename
     sed -i '1d' $filename
     sed -i '1d' $filename
     sed -i '$d' $filename
     sed -i '$d' $filename
     sed -i '$d' $filename
     sed -i '$d' $filename
     sed -i 's/\t//g' $filename
     sed -i "s/^/$3,/g" $filename
elif [ $2 == 'shop' ]
then
	echo 'shop'
	sed -i "s/\`//g" $filename
	sed -i '1d' $filename
elif [ $2 == 'channel' ]
then
    echo 'channel'
	sed -i "s/\`//g" $filename
	sed -i '1d' $filename
fi
