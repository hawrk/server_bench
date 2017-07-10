#!/usr/bin/env python
# coding:utf-8

# 压测程序
# 目前支持tcp和http的get方法

import threading, sys, socket, time, urllib2

ip = "10.100.100.82"
port = 18800
content = "cmd=3&mchid=21313\r\n"
ts = 15
duration = 20
rs={}
isHttp = False

def call_back(index):
    rs[index] = 0
    if isHttp == True:
        # 处理http协议的请求
        while True:
            response = urllib2.urlopen(ip)
            rs[index] += 1

    hd = open(str(index), 'w')
    # 处理tcp协议
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(2)
    result = sock.connect_ex((ip, port))
    if result != 0:
        print "connect error"
        sys.exit(2)
    while True:
        result = sock.sendall(content)
        if result != None:
            print "send error"
        xx = sock.recv(128)
        
        hd.write(xx[19:-1]+'\n')
        rs[index] += 1

if __name__ == "__main__":
    print "ip:", ip, "port:", port, "content:", content, "线程数:", ts, "持续时间:", duration, '秒'
    
    hands = []
    for index in range(ts):
        hands.append(threading.Thread(target=call_back, args=(index,)))
    for thr in hands:
        thr.setDaemon(True)
        thr.start()
    time.sleep(duration)
    count = 0
    for index in range(ts):
        print "线程", index, "共请求",rs[index],"次，平均每秒", rs[index]/duration, "次"
        count += rs[index]
    print "累计请求次数：", count
    print "平均", count/duration, '次/s'
