1. 数据库压测
1.1 执行：./mysql_yace.py [配置文件路径]
如果不指定配置文件，默认是本目录下的sql.xml。
1.2 配置文件说明：
<?xml version="1.0" encoding="utf-8" ?>
<mysql threads="1" duration="5" ip="10.100.100.84" port="3306" dbname="pay_order_db_201703" user="speedpos" passwd="speedpos@123">
    <sql>select 1</sql>
    <sql>select ?(int,6)</sql>
    <sql>select "?(str,3)"</sql>
    <sql>select ?(static,"haha","jjj","xxx","asada")</sql>
</mysql>

threads是线程数；duration是持续时间；sql里是要执行的语句
?(int,6) 在程序执行时会被替换为一个长度为6的随机数字
?(str,3) 在程序执行时会被替换为一个长度为6的随机字符串
?(static,"haha","jjj","xxx","asada") 程序会在"haha","jjj","xxx","asada"这四个字符中随机选择一个
那么这四条sql在程序执行时会被解释为【一种可能】：
select 1
select 2352081
select "OrxCdl"
select "xxx"

2. tcp/http压测
2.1 执行 ./yace.py [配置文件]

2.2 配置
2.2.1 tcp配置
<?xml version="1.0" encoding="utf-8" ?>
<tcp>
    <ip>10.100.100.82</ip>
    <port>18800</port>
    <threads>1</threads>
    <duration>5</duration>
    <content>mchid=?(int,6)&amp;cmd=?(static,1,2)&amp;test=?(str, 6)</content>
</tcp>
&amp; 是&在xml配置中的转义
支持随机化，定义与mysql配置相同

2.2.2 http配置
<?xml version="1.0" encoding="utf-8" ?>
<http>
    <url>http://www.baidu.com/?(int,3)/?(str,5)/?(static,haha,xx,55529665)</url>
    <threads>1</threads>
    <duration>5</duration>
</http>
支持随机化，定义与mysql配置相同
