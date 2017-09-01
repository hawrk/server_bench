#!/bin/bash

sftp_host=$1
sftp_port=$2
sftp_user=$3
sftp_pwd=$4
factor_id=$5
bill_date=$6
remote_path=$7
local_path=$8


lftp -u ${sftp_user},${sftp_pwd} ${sftp_host} <<EOFa
    cd ${remote_path} 
    lcd ${local_path}
    mget *${factor_id}*${bill_date}*
    bye
EOFa


