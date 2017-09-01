# -*- coding:utf-8 -*-
__author__ = 'hawrkchen'
__date__ = '2017/8/07 0022  15:26'
__doc__ = "综合管理平台通道对账单下载"

import sys, socket
import time
import urllib
import os
import logging
import datetime
import json


SERVER_IP = "10.100.120.71"
SERVER_PORT = 40800

bank_list = ["SZPF","GZPF","SZCIB","ECITIC","FJHXBANK"]

def string_msg(bank,date):
    data = {"bill_date":date,"pay_channel":bank}
    biz_content = json.dumps(data)
    content = {}
    content['ver'] = '1.0'
    content['cmd'] = '9020'
    content['src'] = '0'
    content['biz_content'] = biz_content
    str_values = urllib.urlencode(content)
    
    logging.info("content = [%s]",str_values)
    return str_values

#socket send
def send_notify(content):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(20)
    result = sock.connect_ex((SERVER_IP, SERVER_PORT))
    if result != 0:
        #print >>sys.stderr, "connect error, ip:",ip, "port:", port
        sys.exit(2)
    #content = content.replace('\\', '')
    content += '\r\n'
    logging.info("sending data:%s", content)
    result = sock.sendall(content)
    if result != None:
        logging.info("############## time out end#######################")
        #print >>sys.stderr, "send data error, ip:",ip, "port:", port
        sys.exit(3)

    logging.info("receive data:%s" ,sock.recv(128))
    sock.close()

if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s [line:%(lineno)d] %(levelname)s %(message)s',
                        datefmt='%d %b %Y %H:%M:%S',
                        filename='/usr/local/services/spp_route_bill_server-1.0/log/download_bill.log',
                        filemode='a')

    logging.info('*************begin*******************')
    logging.info('notify ip:%s,port :%d',SERVER_IP,SERVER_PORT)
    
    today = datetime.date.today()
    ori_date = today - datetime.timedelta(days= 1)
    proc_date = ori_date.strftime('%Y%m%d')
    logging.info("proc_date = %s",proc_date)
    for bank in bank_list:
        logging.info("proc bank :[%s]",bank)
        send_msg = string_msg(bank,proc_date)
        send_notify(send_msg)
        time.sleep(1)

    logging.info("############## process end#######################")
