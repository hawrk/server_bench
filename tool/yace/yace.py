#!/usr/bin/env python
# coding:utf-8

# 压测程序
# 目前支持tcp和http的get方法

import threading, sys, socket, time, urllib2, xml.dom.minidom, random, string

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
            url = convert_str(ip)
            response = urllib2.urlopen(url)
            rs[index] += 1

    # 处理tcp协议
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(2)
    result = sock.connect_ex((ip, port))
    if result != 0:
        print "connect error"
        sys.exit(2)
    while True:
        send_data = convert_str(content)
        result = sock.sendall(send_data)
        if result != None:
            print "send error"
        xx = sock.recv(128)
        rs[index] += 1

def random_str(len):
    all_let = list(string.ascii_letters)
    random.shuffle(all_let)
    return ''.join(all_let[:len])

def convert_str(str_data):
    while True:
        begin = str_data.find('?')
        if begin == -1:
            break
        if str_data[begin+1] != '(':
            continue
        end = str_data.find(')', begin)
        tmp = str_data[begin:end+1]
        rp_str = ""
        if tmp[2:5] == 'int':
            length = int(tmp[6:len(tmp)-1])
            min = 1
            for i in range(length):
                min *= 10
            xx = random.randint(min, min*10)
            rp_str = str(xx)
        elif tmp[2:5] == 'str':
            length = int(tmp[6:len(tmp)-1])
            rp_str = random_str(length)
        elif tmp[2:8] == 'static':
            statics = tmp[9:len(tmp)-1].split(',')
            rp_str = statics[random.randint(0,len(statics)-1)]
        else:
            print '未识别的关键字', tmp
            sys.exit(1)

        str_data = str_data.replace(tmp, rp_str)
    
    return str_data

if __name__ == "__main__":
    conf_path = "tcp.xml"
    if len(sys.argv) > 1:
        conf_path = sys.argv[1]
    dom = xml.dom.minidom.parse(conf_path)
    root = dom.documentElement

    if root.nodeName == "http":
        isHttp = True
        ip = root.getElementsByTagName("url")[0].firstChild.data
        ts = int(root.getElementsByTagName("threads")[0].firstChild.data)
        duration = int(root.getElementsByTagName("duration")[0].firstChild.data)
        print 'url', ip, "线程数:", ts, "持续时间:", duration, '秒'
    elif root.nodeName == 'tcp':
        ip = root.getElementsByTagName("ip")[0].firstChild.data
        port = int(root.getElementsByTagName("port")[0].firstChild.data)
        content = root.getElementsByTagName("content")[0].firstChild.data
        ts = int(root.getElementsByTagName("threads")[0].firstChild.data)
        duration = int(root.getElementsByTagName("duration")[0].firstChild.data)
        content = root.getElementsByTagName("content")[0].firstChild.data
        print "ip:", ip, "port:", port, "content:", content, "线程数:", ts, "持续时间:", duration, '秒'
    else:
        print '未支持的关键字', root.nodeName
        sys.exit(0)
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
