#!/usr/bin/env python
# coding:utf-8

# 压测MySQL程序
# 
import threading, sys, time, MySQLdb, xml.dom.minidom, random, string

ts = 1 # thread num
duration = 1
rs={}

db_host = ""
db_port = 3306
db_user = ""
db_pwd = ""
db_name = ""
sql = []

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

def call_back(index):
    rs[index] = 0
    conn= MySQLdb.connect(host=db_host, port=db_port, user=db_user, passwd=db_pwd, db =db_name)
    cur = conn.cursor()
    while True:
        for i in range(len(sql)):
            sql_str = convert_str(sql[i].firstChild.data)
            print sql_str
            cur.execute(sql_str)
        conn.commit()
        rs[index] += 1

if __name__ == "__main__":
    conf_path = "sql.xml"
    if len(sys.argv) == 2:
        conf_path = sys.argv[1]
    dom = xml.dom.minidom.parse(conf_path)
    root = dom.documentElement
    ts = int(root.getAttribute("threads"))
    duration = int(root.getAttribute("duration"))
    sql = root.getElementsByTagName("sql")
    db_host = root.getAttribute("ip")
    db_port = int(root.getAttribute("port"))
    db_user = root.getAttribute("user")
    db_pwd = root.getAttribute("passwd")
    db_name = root.getAttribute("dbname")
    print "线程数:", ts, "持续时间:", duration, '秒'
    
    hands = []
    for index in range(ts):
        hands.append(threading.Thread(target=call_back, args=(index,)))
    for thr in hands:
        thr.setDaemon(True)
        thr.start()
    time.sleep(duration)
    count = 0
    for index in range(ts):
        print "线程", index, "共插入数据",rs[index],"条，平均每秒", rs[index]/duration, "条"
        count += rs[index]
    print "累计插入条数：", count
    print "平均", count/duration, '条/s'
