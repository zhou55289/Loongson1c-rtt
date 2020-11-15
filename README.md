# Loongson1c-rtt
本项目基于龙芯1C和RT-THREAD平台开发一套建筑物环境检测及紧急路线设计系统，实现建筑物环境实时监控及紧急情况（火灾）下的疏散路线规划。
代码文件说明
1.aht10-change、bh1750-latest-change、ccs811-latest文件：
使用时将以上文件放入类似于E:\loongson\jingpengzhou-Loongson-Smartloong-V3.0-RTT4.0.2\bsp\ls1cdev\packages
的rrt的bsp龙芯1c的applications或者packages下
2.sensors文件：
对rtt的源码进行了修改，使用将该文件替换原有rt源码\components\drivers\sensor文件即可
