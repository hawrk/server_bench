#!/usr/bin/env python
# encoding utf-8

import xml.dom.minidom
dom = xml.dom.minidom.parse("sql.xml")
root = dom.documentElement
print root.nodeName 
print root.getAttribute("ip")
nodeList = root.getElementsByTagName("sql")
print len(nodeList)
print nodeList[0].firstChild.data
print nodeList[0].getAttribute("flag")
