__author__ = 'hawrkchen'
__date__ = '2017/5/22 0022  15:54'

import sys, socket
import time
import urllib
import os
import logging
import datetime

SERVER_IP = "10.100.100.82"
SERVER_PORT = 13680

bank_list = ["1007","1008"]

def string_msg(bank):
    content = {}
    content['ver'] = '1'
    content['cmd'] = '5040'
    content['src'] = '0'
    content['bm_id'] = bank
    content['pay_channel'] = "WXPAY"
    content["input_time"] = getYesterday()

    str_values = urllib.urlencode(content)
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
    
def getYesterday(): 
    today=datetime.date.today() 
    oneday=datetime.timedelta(days=1) 
    yesterday=today-oneday  
    return yesterday

if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s [line:%(lineno)d] %(levelname)s %(message)s',
                        datefmt='%d %b %Y %H:%M:%S',
                        filename='/usr/local/services/spp_speedpos_bill-2.0/log/reconciliation_wxpay.log',
                        filemode='a')

    logging.info('*************begin*******************')
    logging.info('notify ip:%s,port :%d',SERVER_IP,SERVER_PORT)

    for bank in bank_list:
        logging.info('process bank:%s',bank)
        send_msg = string_msg(bank)
        send_notify(send_msg)
        #logging.info("send_msg =%s", send_msg)
        time.sleep(5)

    logging.info("############## process end#######################")