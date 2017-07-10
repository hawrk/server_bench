#!/usr/bin/env python
# coding:utf-8

# 压测程序
# 目前支持tcp和http的get方法

import threading, sys, socket, time, urllib2

ip = "0.0.0.0"
port = 18800
content = ""
ts = 1
duration = 1
rs={}
isHttp = False

def call_back(index):
    rs[index] = 0
    if isHttp == True:
        # 处理http协议的请求
        while True:
            response = urllib2.urlopen(ip)
            rs[index] += 1

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
        rs[index] += 1

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print 'http Usage: url 线程数 持续时间'
        print "TCP Usage: ip port cmd 线程数 持续时间"
        sys.exit(5)
    ip = sys.argv[1]
    if ip[:4] == 'http':
        isHttp = True
        ts = int(sys.argv[2])
        duration = int(sys.argv[3])
        print 'url', ip, "线程数:", ts, "持续时间:", duration, '秒'
    else:
        if len(sys.argv) < 6:
            print "TCP Usage: ip port cmd 线程数 持续时间"
            sys.exit(4)
        port = int(sys.argv[2])
        content = sys.argv[3]
        ts = int(sys.argv[4])
        duration = int(sys.argv[5])
        print "ip:", ip, "port:", port, "content:", content, "线程数:", ts, "持续时间:", duration, '秒'
    
    hands = []
    for index in range(ts):
        hands.append(threading.Thread(target=call_back, args=(index,)))
    for thr in hands:
        #thr.setDaemon(True)
        thr.start()
    time.sleep(duration)
    count = 0
    for index in range(ts):
        print "线程", index, "共请求",rs[index],"次，平均每秒", rs[index]/duration, "次"
        count += rs[index]
    print "累计请求次数：", count
    print "平均", count/duration, '次/s'
