#!/usr/bin/python2.7
# coding: utf-8
__author__ = 'hawrk'
__date__ = '2017.04.28'
__doc__ = 'for settled notify to billserver 5050 service'
import sys, socket
import logging
import time
import configparser
import urllib
import os

# 示例格式：./trigger.sh 10.100.100.82 16800 mchid=789\&cmd=1  # &前要加\转义

# crontab示例：
# 	* * * * * /home/user_00/spp_3.0.1_release/src/tool/trigger.sh 10.100.100.82 16800 mchid=789\&cmd=1 1>/dev/null 2>>/data/log/trigger.log
# 	# 正常的日子会被丢弃，错误日子会被打印到日志文件里供以后查阅

#通知对账Server
SERVER_IP = "10.100.100.88"
SERVER_PORT = 13680

SETTLE_PATH = "/usr/local/services/spp_speedpos_bill-2.0/client/speedpos_bill/data/paybill/result/"
CONF_PATH = "/usr/local/services/spp_speedpos_bill-2.0/client/speedpos_bill/admin/"
BANK = "1001"
WXCHANNEL = "WXPAYZF"
ALICHANNEL = "ALIPAYZF"

def set_conf(conf_file,curdate):
    cf = configparser.ConfigParser()
    cf.read(conf_file)

    cf.set("conf","date",curdate)
    cf.set("conf","wx_notify_flag","0")
    cf.set("conf","ali_notify_flag","0")
    cf.write(open(conf_file,"w"))

def read_conf(conf_file):
    cf = configparser.ConfigParser()
    cf.read(conf_file)

    cf.sections()
    #option = cf.options("mail")
    date = cf.get("conf","date")
    bank = cf.get("conf","bank")
    wx_notify = cf.getint("conf","wx_notify_flag")
    ali_notify = cf.getint("conf","ali_notify_flag")
    #print ('date =%s' %date)
    #print ('send_flag = %d' %send_flag)
    return date,bank,wx_notify,ali_notify

def set_send_flag(conf_file,channel):
    cf = configparser.ConfigParser()
    cf.read(conf_file)

    if channel == "WXPAYZF":
        cf.set("conf", "wx_notify_flag", "1")
    else:
        cf.set("conf","ali_notify_flag", "1")
    cf.write(open(conf_file, "w"))

def check_file(file_name):
    file = SETTLE_PATH + BANK + "/" + file_name
    logging.info('check file:%s',file)
    if os.path.exists(file):
        return True
    else:
        return False

def string_msg(channel,file_name):
    content = {}
    content['ver'] = '1'
    content['cmd'] = '5050'
    content['src'] = '1'
    content['bm_id'] = BANK
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
                        filename='/usr/local/services/spp_speedpos_bill-2.0/log/settle_notify.log',
                        filemode='a')

    logging.info('*************begin*******************')
    curdate = time.strftime('%Y%m%d', time.localtime(time.time()))
    logging.info('notify ip:%s,port :%d,date = [%s]',SERVER_IP,SERVER_PORT,curdate)

    conf_file = CONF_PATH + BANK + "_notify.conf"
    confdate,bank,wx_notify,ali_notify = read_conf(conf_file)
    if curdate != confdate:
        set_conf(conf_file,curdate)

    #取文件名
    wx_settle_file = BANK + "_" + WXCHANNEL +curdate + ".RET"
    ali_settle_file = BANK + "_" + ALICHANNEL + curdate + ".RET"
    #微信结算单
    if (check_file(wx_settle_file) and wx_notify == 0):
        str_value = string_msg(WXCHANNEL,wx_settle_file)
        send_notify(str_value)
        set_send_flag(conf_file,WXCHANNEL)
    #支付宝结算单
    if(check_file(ali_settle_file) and ali_notify == 0):
        str_value = string_msg(ALICHANNEL,ali_settle_file)
        send_notify(str_value)
        set_send_flag(conf_file,ALICHANNEL)

    logging.info("############## process end#######################")
