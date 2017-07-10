#!/usr/bin/python2.7
# coding: utf-8
import sys, socket
# 示例格式：./trigger.sh 10.100.100.82 16800 mchid=789\&cmd=1  # &前要加\转义

# crontab示例：
# 	* * * * * /home/user_00/spp_3.0.1_release/src/tool/trigger.sh 10.100.100.82 16800 mchid=789\&cmd=1 1>/dev/null 2>>/data/log/trigger.log
# 	# 正常的日子会被丢弃，错误日子会被打印到日志文件里供以后查阅

if __name__ == "__main__":
    print "------- trigger.py ------"
    if len(sys.argv) < 4:
        print >>sys.stderr, "Usage: %s ip port [cmd]" % sys.argv[0]
        sys.exit(1)
    (ip, port, content) = sys.argv[1:4]
    print "ip:", ip, " port:", port, " content:", content
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(2)
    result = sock.connect_ex((ip, int(port)))
    if result != 0:
        print >>sys.stderr, "connect error, ip:",ip, "port:", port
        sys.exit(2)
    #content = content.replace('\\', '')
    content += '\r\n'
    result = sock.sendall(content)
    if result != None:
        print >>sys.stderr, "send data error, ip:",ip, "port:", port
        sys.exit(3)
    print "receive data:", sock.recv(128)
    sock.close()
    sys.exit(0)
