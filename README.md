# dot1x-ConfigService
802.1x配置服务端，采用wpa_supplicant为基础通信层，dot1x-ConfigService为服务数据配置层

本程序总配置了4种通信方式，EAP-PEAP,EAP-MD5,EAP-TLS,EAP-TTLS

本流程为 802.1x EAP-MD5基本通信流程,其他的请百度，这里简略的说一下
![image](https://github.com/tolzlz/dot1x-ConfigService/blob/main/ea40bb2eeef09cde57c5de367a336d70.png)



下面是在安卓端认证的基本流程
![image](https://github.com/tolzlz/dot1x-ConfigService/blob/main/%E6%97%A0%E6%A0%87%E9%A2%982.png)

EAP-MD5认证，部分radius不支持，如radius服务器不返回MD5-challege，导致eap-md5加密错误。radius 服务器支持版本各有差异，请参考官网
http://w1.fi/cgit/hostap/plain/wpa_supplicant/eap_testing.txt
