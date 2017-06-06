#!/usr/bin/python2.7
# coding: utf-8
__author__ = 'hawrk'
__date__ = '2017.04.28'
__doc__ = 'for settled notify to billserver 5050 service'
import sys, socket
import logging
import time
import urllib
import os
import shutil


# 示例格式：./trigger.sh 10.100.100.82 16800 mchid=789\&cmd=1  # &前要加\转义

# crontab示例：
# 	* * * * * /home/user_00/spp_3.0.1_release/src/tool/trigger.sh 10.100.100.82 16800 mchid=789\&cmd=1 1>/dev/null 2>>/data/log/trigger.log
# 	# 正常的日子会被丢弃，错误日子会被打印到日志文件里供以后查阅

#通知对账Server
SERVER_IP = "10.100.100.88"
SERVER_PORT = 13680

SETTLE_PATH = "/usr/local/services/spp_speedpos_bill-2.0/client/speedpos_bill/data/paybill/"
bank_list = ["8966","1001"]
WXCHANNEL = "WXPAY"
ALICHANNEL = "ALIPAY"

def string_msg(channel,file_name,bank):
    content = {}
    content['ver'] = '1'
    content['cmd'] = '5050'
    content['src'] = '1'
    content['bm_id'] = bank
    content['pay_channel'] = channel
    content['file_name'] = file_name

    str_values = urllib.urlencode(content)
    return str_values

#socket发送
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
                        filename='/usr/local/services/spp_speedpos_bill-2.0/log/settle_notify2.log',
                        filemode='a')

    logging.info('*************begin*******************')
    logging.info('notify ip:%s,port :%d',SERVER_IP,SERVER_PORT)

    for bank in bank_list:
        settle_path = SETTLE_PATH + bank + "/result/"
        logging.info('process bank:%s',bank)
        for file in os.listdir(settle_path):
            logging.info('file name [%s]',file)
            channel = file[-19:-14]
            logging.info('channel name [%s]', channel)
            #微信结算单
            if (channel == "WXPAY"):
                str_value = string_msg(WXCHANNEL,file,bank)
                send_notify(str_value)
            #支付宝结算单
            else:
                str_value = string_msg(ALICHANNEL,file,bank)
                send_notify(str_value)
            #src,dest
            #后续处理放在c++端处理
            #shutil.move(TEMP_PATH+file,SETTLE_PATH+file)
            #os.remove(os.path.join(TEMP_PATH,file))

    logging.info("############## process end#######################")
