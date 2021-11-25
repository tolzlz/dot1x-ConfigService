#include<stdio.h>
#include<string>
#include<stdlib.h>
#include<iostream>
#include<fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include<netinet/in.h>
//#include <libnet.h>
#include <map>
#include "wiredclient.h"


using namespace std;
void ModifyLineData(char* fileName, int lineNum, char* lineData);
std::string charToStr(char * contentChar);
void modifyContentInFile(char const *fileName,int lineNum,char *content);
#define BUFLEN 1024

#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <vector>
#include <sstream>
#include <iterator>



#define BUFLEN 1024
#define COMMENT_CHAR '#'

#define  LOG_TAG    "KuiTag"
#define  LOGD_SERVER(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

bool IsSpace(char c)
{
    if (' ' == c || '\t' == c)
        return true;
    return false;
}

void Trim(string & str)
{
    if (str.empty()) {
        return;
    }
    int i, start_pos, end_pos;
    for (i = 0; i < str.size(); ++i) {
        if (!IsSpace(str[i])) {
            break;
        }
    }
    if (i == str.size()) { // 全部是空白字符串
        str = "";
        return;
    }

    start_pos = i;

    for (i = str.size() - 1; i >= 0; --i) {
        if (!IsSpace(str[i])) {
            break;
        }
    }
    end_pos = i;

    str = str.substr(start_pos, end_pos - start_pos + 1);
}



bool AnalyseLine(const string & line, string & key, string & value)
{
    if (line.empty())
        return false;
    int start_pos = 0, end_pos = line.size() - 1, pos;
    if ((pos = line.find(COMMENT_CHAR)) != -1) {
        if (0 == pos) {  // 行的第一个字符就是注释字符
            return false;
        }
        end_pos = pos - 1;
    }
    string new_line = line.substr(start_pos, start_pos + 1 - end_pos);  // 预处理，删除注释部分

    if ((pos = new_line.find('=')) == -1)
        return false;  // 没有=号

    key = new_line.substr(0, pos);
    value = new_line.substr(pos + 1, end_pos + 1- (pos + 1));

    Trim(key);
    if (key.empty()) {
        return false;
    }
    Trim(value);
    return true;
}


bool ReadConfig(const string & filename, map<string, string> & m)
{
    m.clear();
    ifstream infile(filename.c_str());
    if (!infile) {
        cout << "file open error" << endl;
        return false;
    }
    string line, key, value;
    while (getline(infile, line)) {
        if (AnalyseLine(line, key, value)) {
            m[key] = value;
        }
    }

    infile.close();
    return true;
}

bool ReadConfigStream(const string & buf, map<string, string> & m)
{
//    m.clear();
//    ifstream infile(filename.c_str());
//    if (!infile) {
//        cout << "file open error" << endl;
//        return false;
//    }
//    string line, key, value;
//    while (getline(infile, line)) {
//        if (AnalyseLine(line, key, value)) {
//            m[key] = value;
//        }
//    }
//
//    infile.close();
//    return true;




    m.clear();
    std::string data = buf;
    std::istringstream data_istream(data);
    std::string buffer;
    std::vector<std::string> result;
    string line, key, value;

    while (std::getline(data_istream,line))
    {
        if (AnalyseLine(line, key, value)) {
            m[key] = value;
        }
    }
    return true;

}
void PrintConfig(const map<string, string> & m)
{
    map<string, string>::const_iterator mite = m.begin();
    for (; mite != m.end(); ++mite) {
        cout << mite->first << "=" << mite->second << endl;
    }
}



int main()
{


    int sockfd, newfd;
    struct sockaddr_in s_addr, c_addr;
    char buf[BUFLEN];
    socklen_t len;
    unsigned int port, listnum;
    fd_set rfds;
    struct timeval tv;
    int retval,maxfd;

    /*建立socket*/
    //sleep(2);




    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket");
        exit(errno);
    }else
        printf("socket create success!\n");
        //sleep(2);
    /*设置服务器端口*/
    port = 4567;
    /*设置侦听队列长度*/
    listnum = 3;
    /*设置服务器ip*/
    bzero(&s_addr, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(port);
    s_addr.sin_addr.s_addr = INADDR_ANY;
    /*把地址和端口帮定到套接字上*/
    if((::bind(sockfd, (struct sockaddr*) &s_addr,sizeof(struct sockaddr))) == -1){
        perror("bind");
        exit(errno);
    }else
        printf("bind success!\n");
        //sleep(2);

    /*侦听本地端口*/
    if(listen(sockfd,listnum) == -1){
        perror("listen");
        exit(errno);
    }else
        printf("the server is listening!\n");
        //sleep(2);
    while(1){
        printf("*****************聊天开始***************\n");
        len = sizeof(struct sockaddr);
        if((newfd = accept(sockfd,(struct sockaddr*) &c_addr, &len)) == -1){
            perror("accept");
            exit(errno);
        }else
            printf("正在与您聊天的客户端是：%s: %d\n",inet_ntoa(c_addr.sin_addr),ntohs(c_addr.sin_port));
            //sleep(2);
        while(1){
            /*把可读文件描述符的集合清空*/
            FD_ZERO(&rfds);
            /*把标准输入的文件描述符加入到集合中*/
            FD_SET(0, &rfds);
            maxfd = 0;
            /*把当前连接的文件描述符加入到集合中*/
            FD_SET(newfd, &rfds);
            /*找出文件描述符集合中最大的文件描述符*/
            if(maxfd < newfd)
                maxfd = newfd;
            /*设置超时时间*/
            tv.tv_sec = 5;
            tv.tv_usec = 0;
            /*等待聊天*/
            retval = select(maxfd+1, &rfds, NULL, NULL, &tv);
            if(retval == -1){
                printf("select出错，与该客户端连接的程序将退出\n");
                break;
            }else if(retval == 0){
                printf("服务器没有任何输入信息，并且客户端也没有信息到来，waiting...\n");
                //sleep(2);
                continue;
            }else{
                /*用户输入信息了,开始处理信息并发送*/
//                if(FD_ISSET(0, &rfds)){
//                    _retry:
//                    /******发送消息*******/
//                    bzero(buf,BUFLEN);
//                    /*fgets函数：从流中读取BUFLEN-1个字符*/
//                    fgets(buf,BUFLEN,stdin);
//                    /*打印发送的消息*/
//                    //fputs(buf,stdout);
//                    if(!strncasecmp(buf,"quit",4)){
//                        printf("server 请求终止聊天!\n");
//                        break;
//                    }
//                    /*如果输入的字符串只有"\n"，即回车，那么请重新输入*/
//                    if(!strncmp(buf,"\n",1)){
//                        printf("输入的字符只有回车，这个是不正确的！！！\n");
//                        goto _retry;
//                    }
//                    /*如果buf中含有'\n'，那么要用strlen(buf)-1，去掉'\n'*/
//                    if(strchr(buf,'\n'))
//                        len = send(newfd,buf,strlen(buf)-1,0);
//                        /*如果buf中没有'\n'，则用buf的真正长度strlen(buf)*/
//                    else
//                        len = send(newfd,buf,strlen(buf),0);
//                    if(len > 0)
//                        printf("\t消息发送成功，本次共发送的字节数是：%d\n",len);
//                    else{
//                        printf("消息发送失败!\n");
//                        break;
//                    }
//                }
                /*客户端发来了消息*/
                if(FD_ISSET(newfd, &rfds)){
                    /******接收消息*******/
                    bzero(buf,BUFLEN);
                    len = recv(newfd,buf,BUFLEN,0);
                    char *token;
                    char *reply_str;
                    if(len > 0) {
//                        printf("客户端发来的信息是：%s\n,共有字节数是: %d\n", buf, len);
                        string recv_str = buf;
                        string dd = "接收到字符串"+recv_str;
                        const char * p = dd.data();

                        map<string,string> m;
                        ReadConfigStream(buf, m);
                        //PrintConfig(m);
                        map<string, string>::const_iterator mite = m.begin();
                        //cout << mite->first << "=" << mite->second << endl;

                        for (; mite != m.end(); ++mite)
                        {
                            if(mite->first.compare("eap") == 0)
                            {
                                string hh = mite->first;
                                string jj = mite->second;
                                dd = "获取到首部"+ hh;
                                dd = "获取到尾部"+ jj;

                                if (mite->second.compare("md5") == 0)
                                {
                                    cout << "md5" <<endl;
                                    std::string identity;
                                    std::string password;
                                  
                                    for (mite = m.begin(); mite != m.end(); mite++) {
                                        //cout << mite->first << "=" << mite->second << endl;
                                        if (mite->first.compare("identity") == 0)
                                        {
                                            identity = mite->second;
                                        };
                                        if(mite->first.compare("password") == 0)
                                        {
                                            password = mite->second;
                                        }
                                    }
                                    if(!identity.empty() || !identity.empty())
                                    {
                                        wiredclient client;
                                        if(!client.GetInitStatus()) {
                                            std::cout << "wpa client init failed!!!\n";
//                                            return -1;
                                            break;
                                        }
                                        bool result = client.ConnectWiredWithMD5(identity,password);
                                        if (!result)
                                        {
                                            printf("设置项失败");
                                            return -1;
                                        }
                                        int time_out = 15000;  //ms
                                        std::string recv;
                                        while (!client.GetConnectStatus(recv))
                                        {
                                            bool bStop = false;
                                            //sleep(5);
                                            std::cout<<"recv str :"<<recv<<std::endl;
                                            map<string,string> recv_str;
                                            ReadConfigStream(recv, recv_str);
                                            //PrintConfig(m);
                                            map<string, string>::const_iterator mitus = recv_str.begin();
                                            for (mitus = recv_str.begin(); mitus != recv_str.end(); mitus++) {
                                                if (mitus->first.compare("EAP state") == 0)
                                                {
                                                    if (mitus->second.compare("FAILURE") == 0)
                                                    {
                                                        char reply_str[50] = "13";
                                                        int send_le;
                                                        if((send_le = send(newfd,reply_str, strlen(reply_str),0)) > 0)
                                                        {
                                                            printf("服务券向客户端发送大小为 %d\n",send_le);
                                                            bStop = true;
        						    break;
                                                        } else{
                                                            printf("服务券向客户端发送失败了 %d\n");
                                                        };
                                                    } else if (mitus->second.compare("SUCCESS") == 0){
                                                        char reply_str[50] = "12";
                                                        int send_le;
                                                        if((send_le = send(newfd,reply_str, strlen(reply_str),0)) > 0)
                                                        {
                                                            printf("服务券向客户端发送大小为 %d\n",send_le);
                                                            //现在获取IP地址
                                                            bStop = true;
        						    break;
                                                        } else{
                                                            printf("服务券向客户端发送失败了 %d\n");
                                                        };
                                                    } else if (mitus->second.compare("IDLE") == 0){
                                                        //char reply_str[50]="10";
                                                        //int send_le;
                                                        //if((send_le = send(newfd,reply_str, strlen(reply_str),0)) > 0)
                                                        //{
                                                         //   printf("服务券向客户端发送大小为 %d\n",send_le);
                                                        //} else{
                                                        //    printf("服务券向客户端发送失败了 %d\n");
                                                        //};
                                                    }
                                                }
                                            }
                                            if (bStop)
					    {
					    	break;
					    }
                                        }
                                    } else{
                                        char reply_str[50] = "01";
                                        send(newfd,reply_str,strlen(reply_str),0);
                                    }
                                } else if (mite->second.compare("peap") == 0)
                                {
                                    cout << "peap" <<endl;
                                    std::string identity;
                                    std::string password;
                                    std::string ca_cert;
                                    std::string usedhcp;
                                    for (mite = m.begin(); mite != m.end(); mite++) {
                                        //cout << mite->first << "=" << mite->second << endl;
                                        if (mite->first.compare("identity") == 0)
                                        {
                                            identity = mite->second;
                                        };
                                        if(mite->first.compare("password") == 0)
                                        {
                                            password = mite->second;
                                        }
                                        if (mite->first.compare("ca_cert") == 0)
                                        {
                                            ca_cert = mite->second;
                                        }
                                    }
                                    if(!identity.empty() || !password.empty() || !ca_cert.empty())
                                    {
                                        wiredclient client;
                                        if(!client.GetInitStatus()) {
                                            std::cout << "wpa client init failed!!!\n";

//                                            return -1;
                                            break;
                                        }
                                        int result = client.ConnectWiredWithPEAP(identity,password,ca_cert);
                                        if (!result)
                                        {
                                            printf("设置项失败");
                                            return -1;
                                        }
                                        int time_out = 15000;  //ms
                                        std::string recv;
                                        while (!client.GetConnectStatus(recv))
                                        {
                                            bool bStop = false;
                                            //sleep(5);
                                            //std::cout<<"recv str :"<<recv<<std::endl;
                                            map<string,string> recv_str;
                                            ReadConfigStream(recv, recv_str);
//                                            //PrintConfig(m);
                                            map<string, string>::const_iterator mitus = recv_str.begin();
                                            for (mitus = recv_str.begin(); mitus != recv_str.end(); mitus++) {
                                                if (mitus->first.compare("EAP state") == 0)
                                                {
                                                    if (mitus->second.compare("FAILURE") == 0)
                                                    {
                                                        char reply_str[50] = "13";
                                                        int send_le;
                                                        if((send_le = send(newfd,reply_str, strlen(reply_str),0)) > 0)
                                                        {
                                                            printf("服务券向客户端发送大小为 %d\n",send_le);
                                                            bStop = true;
        						    break;
                                                        } else{
                                                            printf("服务券向客户端发送失败了 %d\n");
                                                        };
                                                    } else if (mitus->second.compare("SUCCESS") == 0){
                                                        char reply_str[50] = "12";
                                                        int send_le;
                                                        if((send_le = send(newfd,reply_str, strlen(reply_str),0)) > 0)
                                                        {
                                                            printf("服务券向客户端发送大小为 %d\n",send_le);
                                                            bStop = true;
        						    break;
                                                        } else{
                                                            printf("服务券向客户端发送失败了 %d\n");
                                                        };
                                                    } else if (mitus->second.compare("IDLE") == 0){
                                                        //char reply_str[50]="10";
                                                        //int send_le;
                                                        //if((send_le = send(newfd,reply_str, strlen(reply_str),0)) > 0)
                                                        //{
                                                        //    printf("服务券向客户端发送大小为 %d\n",send_le);
                                                        //} else{
                                                        //    printf("服务券向客户端发送失败了 %d\n");
                                                        //};
                                                    }
                                                }
                                            }
                                            if (bStop)
					    {
					    	break;
					    }
                                        }
                                    } else{
                                        char reply_str[50] = "01";
                                        send(newfd,reply_str,strlen(reply_str),0);
                                    }
                                } else if (mite->second.compare("ttls") == 0)
                                {
                                    cout << "ttls" <<endl;
                                    std::string identity;
                                    std::string password;
                                    std::string ca_cert;
                                    std::string anonymous_identity;
                                    std::string usedhcp;
                                    for (mite = m.begin(); mite != m.end(); mite++) {
                                        //cout << mite->first << "=" << mite->second << endl;
                                        if (mite->first.compare("identity") == 0)
                                        {
                                            identity = mite->second;
                                        };
                                        if(mite->first.compare("password") == 0)
                                        {
                                            password = mite->second;
                                        }
                                        if (mite->first.compare("ca_cert") == 0)
                                        {
                                            ca_cert = mite->second;
                                        }
                                        if (mite->first.compare("anonymous_identity") == 0)
                                        {
                                            anonymous_identity = mite->second;
                                        }
                                    }
                                    if(!identity.empty() || !password.empty() || !ca_cert.empty() || !anonymous_identity.empty())
                                    {
                                        wiredclient client;
                                        if(!client.GetInitStatus()) {
                                            std::cout << "wpa client init failed!!!\n";
                                            return -1;
                                        }
                                        bool  result = client.ConnectWiredWithTTLS(identity,anonymous_identity,password,ca_cert);
                                        if (!result)
                                        {
                                            printf("设置项失败");
                                            return -1;
                                        }
                                        int time_out = 15000;  //ms
                                        std::string recv;
                                        while (!client.GetConnectStatus(recv))
                                        {
                                            bool bStop = false;
                                            //sleep(5);
                                            std::cout<<"recv str :"<<recv<<std::endl;
                                            map<string,string> recv_str;
                                            ReadConfigStream(recv, recv_str);
                                            //PrintConfig(m);
                                            map<string, string>::const_iterator mitus = recv_str.begin();
                                            for (mitus = recv_str.begin(); mitus != recv_str.end(); mitus++) {
                                                if (mitus->first.compare("EAP state") == 0)
                                                {
                                                    if (mitus->second.compare("FAILURE") == 0)
                                                    {
                                                        char reply_str[50] = "13";
                                                        int send_le;
                                                        if((send_le = send(newfd,reply_str, strlen(reply_str),0)) > 0)
                                                        {
                                                            printf("服务券向客户端发送大小为 %d\n",send_le);
                                                            bStop = true;
        						    break;
                                                        } else{
                                                            printf("服务券向客户端发送失败了 %d\n");
                                                        };
                                                    } else if (mitus->second.compare("SUCCESS") == 0){
                                                        char reply_str[50] = "12";
                                                        int send_le;
                                                        if((send_le = send(newfd,reply_str, strlen(reply_str),0)) > 0)
                                                        {
                                                            printf("服务券向客户端发送大小为 %d\n",send_le);
                                                            bStop = true;
        						    break;
                                                        } else{
                                                            printf("服务券向客户端发送失败了 %d\n");
                                                        };
                                                    } else if (mitus->second.compare("IDLE") == 0){
                                                        //char reply_str[50]="10";
                                                        //int send_le;
                                                        //if((send_le = send(newfd,reply_str, strlen(reply_str),0)) > 0)
                                                        //{
                                                        //    printf("服务券向客户端发送大小为 %d\n",send_le);
                                                        //} else{
                                                        //    printf("服务券向客户端发送失败了 %d\n");
                                                        //};
                                                    }
                                                }
                                            }
                                            if (bStop)
					    {
					    	break;
					    }
                                        }
                                    } else{
                                        char reply_str[50] = "01";
                                        send(newfd,reply_str,strlen(reply_str),0);
                                    }
                                } else if (mite->second.compare("tls") == 0)
                                {
                                    cout << "tls" <<endl;
                                    std::string identity;
                                    std::string ca_cert;
                                    std::string client_cert;
                                    std::string private_key;
                                    std::string private_key_passwd;
                                    std::string usedhcp;
                                    for (mite = m.begin(); mite != m.end(); mite++) {
                                        //cout << mite->first << "=" << mite->second << endl;
                                        if (mite->first.compare("identity") == 0)
                                        {
                                            identity = mite->second;
                                        };
                                        if (mite->first.compare("ca_cert") == 0)
                                        {
                                            ca_cert = mite->second;
                                        }
                                        if (mite->first.compare("client_cert") == 0)
                                        {
                                            client_cert = mite->second;
                                        }
                                        if (mite->first.compare("private_key") == 0)
                                        {
                                            private_key = mite->second;
                                        }
                                        if (mite->first.compare("private_key_passwd") == 0)
                                        {
                                            private_key_passwd = mite->second;
                                        }
                                    }
                                    if(!identity.empty() || !ca_cert.empty() || !client_cert.empty() || !private_key.empty() || !private_key_passwd.empty())
                                    {
                                        wiredclient client;
                                        if(!client.GetInitStatus()) {
                                            std::cout << "wpa client init failed!!!\n";
                                            return -1;
                                        }
                                        bool result = client.ConnectWiredWithTLS(identity,ca_cert,client_cert,private_key,private_key_passwd);
                                        if (!result)
                                        {
                                            printf("设置项失败");
                                            return -1;
                                        }
                                        int time_out = 15000;  //ms
                                        std::string recv;
                                        while (!client.GetConnectStatus(recv))
                                        {
                                            bool bStop = false;
                                            //sleep(5);
                                            std::cout<<"recv str :"<<recv<<std::endl;
                                            map<string,string> recv_str;
                                            ReadConfigStream(recv, recv_str);
                                            //PrintConfig(m);
                                            map<string, string>::const_iterator mitus = recv_str.begin();
                                            for (mitus = recv_str.begin(); mitus != recv_str.end(); mitus++) {
                                                if (mitus->first.compare("EAP state") == 0)
                                                {
                                                    if (mitus->second.compare("FAILURE") == 0)
                                                    {
                                                        char reply_str[50] = "13";
                                                        int send_le;
                                                        if((send_le = send(newfd,reply_str, strlen(reply_str),0)) > 0)
                                                        {
                                                            printf("服务券向客户端发送大小为 %d\n",send_le);
                                                            bStop = true;
        						    break;
                                                        } else{
                                                            printf("服务券向客户端发送失败了 %d\n");
                                                        };
                                                    } else if (mitus->second.compare("SUCCESS") == 0){
                                                        char reply_str[50] = "12";
                                                        int send_le;
                                                        if((send_le = send(newfd,reply_str, strlen(reply_str),0)) > 0)
                                                        {
                                                            printf("服务券向客户端发送大小为 %d\n",send_le);
                                                            bStop = true;
        						    break;
                                                        } else{
                                                            printf("服务券向客户端发送失败了 %d\n");
                                                        };
                                                    } else if (mitus->second.compare("IDLE") == 0){
                                                        //char reply_str[50]="10";
                                                        //int send_le;
                                                        //if((send_le = send(newfd,reply_str, strlen(reply_str),0)) > 0)
                                                        //{
                                                        //    printf("服务券向客户端发送大小为 %d\n",send_le);
                                                        //} else{
                                                        //    printf("服务券向客户端发送失败了 %d\n");
                                                        //};
                                                    }
                                                }
                                            }
                                            if (bStop)
					    {
					    	break;
					    }
                                        }
                                    } else{
                                        char reply_str[50] = "01";
                                        send(newfd,reply_str,strlen(reply_str),0);
                                    }
                                }

                            }
                        };







//                        for (; mite != m.end(); ++mite) {
                            //cout << mite->first << "=" << mite->second << endl;
//                        }

                        //----------------------------------------
//                        token = strtok(buf,"\n");
//                        while (token != nullptr)
//                        {
//                            printf("-------%s \n",token);
//                            token = strtok(nullptr,"\n");
//                        }
                        //---------------------------------------
//                        while (token != nullptr)
//                        {
//                            printf("-------%s \n",token);
//                            token = strtok(nullptr,"\n");
//                        }
                        //----------------------------------------

//                        std::string data = buf;
//                        std::istringstream data_istream(data);
//                        std::string buffer;
//                        std::vector<std::string> result;
//                        while (std::getline(data_istream,buffer))
//                        {
//                            result.push_back(buffer);
//                        }
//                        std::copy(result.begin(),result.end(),std::ostream_iterator<std::string>(std::cout,"\n"));
                    }else{
                        if(len < 0 )
                            printf("接受消息失败！\n");
                        else
                            printf("客户端退出了，聊天终止！\n");
                        break;
                    }
                }
            }
        }
        /*关闭聊天的套接字*/
        close(newfd);
        /*是否退出服务器*/
        printf("服务器是否退出程序：y->是；n->否? ");
        bzero(buf, BUFLEN);
        fgets(buf,BUFLEN, stdin);
        if(!strncasecmp(buf,"y",1)){
            printf("server 退出!\n");
            break;
        }
    }
    /*关闭服务器的套接字*/
    close(sockfd);
    return 0;
}



//    char username[60];
//    char password[60];
//    printf("argv[1]=%s len=%d\n",argv[1],strlen(argv[1]));
//    printf("argv[2]=%s len=%d\n",argv[2],strlen(argv[2]));
//    sprintf(username,"identity=\"%s\"",argv[1]);//第一个参数为名称
//    sprintf(password,"password=\"%s\"",argv[2]);//第二个参数为密码
//    printf("config:%s  %s \n",username,password);
//    printf("Hello, World!\n");
//    modifyContentInFile("./wpa_supplicant.wired.conf",7,username);//修改配置文件的第5行wifi名称
//    modifyContentInFile("./wpa_supplicant.wired.conf",8,password);//修改第7行wifi密码
//    //system("ifconfig wlan0 up");//打开wifi
//    //system("wpa_supplicant -D nl80211 -i wlan0 -c /etc/wpa_supplicant.conf -B");
//    //system("udhcpc -i wlan0");//自动分配ip
//    return 0;

/************************************************************************/
/* char*tostr  字符串转化str类型
输入：char * 字符串地址
无输出
返回值： str类型的字符串
*/
/************************************************************************/
std::string charToStr(char * contentChar)
{
    std::string tempStr;
    for (int i=0;contentChar[i]!='\0';i++)
    {
        tempStr+=contentChar[i];
    }
    return tempStr;
}
/* 修改文件某行内容
 输入：文件名 fileName   行号   lineNum ,修改的内容 content
 输出：文件名 fileName
 无返回值
 tip：1,lineNum从第一行开始 2.content需要加上换行符
*/
/************************************************************************/
void modifyContentInFile(char const *fileName,int lineNum,char *content)
{
    std::ifstream in;
    char line[1024]={'\0'};
    in.open(fileName);
    int i=0;
    std::string tempStr;
    while(in.getline(line,sizeof(line)))
    {
        i++;
        if(lineNum!=i)
        {
            tempStr+=charToStr(line);
        }
        else
        {
            tempStr+=charToStr(content);
        }
        tempStr+='\n';
    }
    in.close();
    std::ofstream out;
    out.open(fileName);
    out.flush();
    out<<tempStr;
    out.close();
}

void ModifyLineData(char* fileName, int lineNum, char* lineData)
{
    ifstream in;
    in.open(fileName);
    string strFileData = "";
    int line = 1;
    char tmpLineData[1024] = {0};
    while(in.getline(tmpLineData, sizeof(tmpLineData)))
    {
        if (line == lineNum)
        {
            strFileData += string(lineData);
            strFileData += "\n";
        }
        else
        {
            strFileData += string(tmpLineData);
            strFileData += "\n";
        }
        line++;
    }
    in.close();
    //写入文件
    ofstream out;
    out.open(fileName);
    out.flush();
    out<<strFileData;
    out.close();
}



