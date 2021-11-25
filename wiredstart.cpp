#include<stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include <sys/select.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include <string>
#include <map>
#include <iostream>
#include "wiredclient.h"
#include <iterator>
#include <vector>
#include <sstream>

#define BACKLOG 5     //完成三次握手但没有accept的队列的长度
#define CONCURRENT_MAX 8   //应用层同时可以处理的连接
#define SERVER_PORT 11332
#define BUFFER_SIZE 1024
#define QUIT_CMD ".quit"
int client_fds[CONCURRENT_MAX];  //声明一个数组来存储状态
#define COMMENT_CHAR '#'

using namespace std;

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


using namespace std;
int main(int argc, const char * argv[])
{
    char input_msg[BUFFER_SIZE];  //限定最大值
    char recv_msg[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    bzero(&(server_addr.sin_zero), 8);
    //创建socket
    int server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock_fd == -1)
    {
        perror("socket error");
        return 1;
    }
    //绑定socket
    int bind_result = bind(server_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(bind_result == -1)
    {
        perror("bind error");
        return 1;
    }
    //listen
    if(listen(server_sock_fd, BACKLOG) == -1)
    {
        perror("listen error");
        return 1;
    }
    //fd_set
    fd_set server_fd_set;  /*fd_set实际上是一long类型的数组，每一个数组元素都能与一打开的文件句柄 (不管是socket句柄，还是其他文件或命名管道或设备句柄) 建立联系，建立联系的工作由程序员完成，当调用select()时，由内核根据IO状态修改fe_set的内容，由此来通知执行了select()的进程哪一socket或文件可读。*/
    int max_fd = -1;
    struct timeval tv;  //超时时间设置
    while(1)
    {
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        FD_ZERO(&server_fd_set);
        FD_SET(STDIN_FILENO, &server_fd_set);
        if(max_fd <STDIN_FILENO)  //STDIN_FILENO 标准输入设备的文件描述符（键盘）
        {
            max_fd = STDIN_FILENO;
        }
        //printf("STDIN_FILENO=%d\n", STDIN_FILENO);
        //服务器端socket
        FD_SET(server_sock_fd, &server_fd_set);
        // printf("server_sock_fd=%d\n", server_sock_fd);
        if(max_fd < server_sock_fd)
        {
            max_fd = server_sock_fd;
        }
        //客户端连接
        for(int i =0; i < CONCURRENT_MAX; i++)
        {
            //printf("client_fds[%d]=%d\n", i, client_fds[i]);
            if(client_fds[i] != 0)
            {
                FD_SET(client_fds[i], &server_fd_set);
                if(max_fd < client_fds[i])
                {
                    max_fd = client_fds[i];
                }
            }
        }
        int ret = select(max_fd + 1, &server_fd_set, NULL, NULL, &tv);
        if(ret < 0)
        {
            perror("select 出错\n");
            continue;
        }
        else if(ret == 0)
        {
            printf("select 超时\n");
            continue;
        }
        else
        {
            //ret 为未状态发生变化的文件描述符的个数
            if(FD_ISSET(STDIN_FILENO, &server_fd_set))
            {
                printf("发送消息：\n");
                bzero(input_msg, BUFFER_SIZE);
                fgets(input_msg, BUFFER_SIZE, stdin);
                //输入“.quit"则退出服务器
                if(strcmp(input_msg, QUIT_CMD) == 0)
                {
                    exit(0);
                }
                for(int i = 0; i < CONCURRENT_MAX; i++)
                {
                    if(client_fds[i] != 0)
                    {
                        printf("client_fds[%d]=%d\n", i, client_fds[i]);
                        send(client_fds[i], input_msg, BUFFER_SIZE, 0);
                    }
                }
            }
            if(FD_ISSET(server_sock_fd, &server_fd_set))
            {
                //有新的连接请求
                struct sockaddr_in client_address;
                socklen_t address_len;
                int client_sock_fd = accept(server_sock_fd, (struct sockaddr *)&client_address, &address_len);
                printf("new connection client_sock_fd = %d\n", client_sock_fd);
                if(client_sock_fd > 0)
                {
                    int index = -1;
                    for(int i = 0; i < CONCURRENT_MAX; i++)
                    {
                        if(client_fds[i] == 0)
                        {
                            index = i;
                            client_fds[i] = client_sock_fd;
                            break;
                        }
                    }
                    if(index >= 0)
                    {
                        printf("新客户端(%d)加入成功 %s:%d\n", index, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                    }
                    else
                    {
                        bzero(input_msg, BUFFER_SIZE);
                        strcpy(input_msg, "服务器加入的客户端数达到最大值,无法加入!\n");
                        send(client_sock_fd, input_msg, BUFFER_SIZE, 0);
                        printf("客户端连接数达到最大值，新客户端加入失败 %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                    }
                }
            }
            for(int i =0; i < CONCURRENT_MAX; i++)
            {
                if(client_fds[i] !=0)
                {
                    if(FD_ISSET(client_fds[i], &server_fd_set))
                    {
                        //处理某个客户端过来的消息
                        bzero(recv_msg, BUFFER_SIZE);
                        long byte_num = recv(client_fds[i], recv_msg, BUFFER_SIZE, 0);
                        if (byte_num > 0)
                        {
//                            if(byte_num > BUFFER_SIZE)
//                            {
//                                byte_num = BUFFER_SIZE;
//                            }
//                            recv_msg[byte_num] = '\0';
                            printf("客户端(%d):%s\n", i, recv_msg);
                            std::string recv_str = recv_msg;
                            map<string,string> m;
                            ReadConfigStream(recv_msg, m);
                            //PrintConfig(m);
                            map<string, string>::const_iterator mite = m.begin();
                            //cout << mite->first << "=" << mite->second << endl;

                            for (; mite != m.end(); ++mite)
                            {
                                if(mite->first.compare("eap") == 0)
                                {
                                    string hh = mite->first;
                                    string jj = mite->second;

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
                                                            if((send_le = send(client_fds[i],reply_str, strlen(reply_str),0)) > 0)
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
                                                            if((send_le = send(client_fds[i],reply_str, strlen(reply_str),0)) > 0)
                                                            {
                                                                printf("服务券向客户端发送大小为 %d\n",send_le);
                                                                bStop = true;
                                                                break;
                                                            } else{
                                                                printf("服务券向客户端发送失败了 %d\n");
                                                            };
                                                        } else if (mitus->second.compare("IDLE") == 0){
//                                                        char reply_str[50]="10";
//                                                        int send_le;
//                                                        if((send_le = send(client_fds[i],reply_str, strlen(reply_str),0)) > 0)
//                                                        {
//                                                            printf("服务券向客户端发送大小为 %d\n",send_le);
//                                                        } else{
//                                                            printf("服务券向客户端发送失败了 %d\n");
//                                                        };
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
                                            send(client_fds[i],reply_str,strlen(reply_str),0);
                                        }
                                    } else if (mite->second.compare("peap") == 0)
                                    {
                                        cout << "peap" <<endl;
                                        std::string identity;
                                        std::string password;
                                        std::string ca_cert;
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
                                                            if((send_le = send(client_fds[i],reply_str, strlen(reply_str),0)) > 0)
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
                                                            if((send_le = send(client_fds[i],reply_str, strlen(reply_str),0)) > 0)
                                                            {
                                                                printf("服务券向客户端发送大小为 %d\n",send_le);
                                                                bStop = true;
                                                                break;
                                                            } else{
                                                                printf("服务券向客户端发送失败了 %d\n");
                                                            };
                                                        } else if (mitus->second.compare("IDLE") == 0){
//                                                        char reply_str[50]="10";
//                                                        int send_le;
//                                                        if((send_le = send(client_fds[i],reply_str, strlen(reply_str),0)) > 0)
//                                                        {
//                                                            printf("服务券向客户端发送大小为 %d\n",send_le);
//                                                        } else{
//                                                            printf("服务券向客户端发送失败了 %d\n");
//                                                        };
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
                                            send(client_fds[i],reply_str,strlen(reply_str),0);
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
                                                            if((send_le = send(client_fds[i],reply_str, strlen(reply_str),0)) > 0)
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
                                                            if((send_le = send(client_fds[i],reply_str, strlen(reply_str),0)) > 0)
                                                            {
                                                                printf("服务券向客户端发送大小为 %d\n",send_le);
                                                                bStop = true;
                                                                break;
                                                            } else{
                                                                printf("服务券向客户端发送失败了 %d\n");
                                                            };
                                                        } else if (mitus->second.compare("IDLE") == 0){
//                                                        char reply_str[50]="10";
//                                                        int send_le;
//                                                        if((send_le = send(client_fds[i],reply_str, strlen(reply_str),0)) > 0)
//                                                        {
//                                                            printf("服务券向客户端发送大小为 %d\n",send_le);
//                                                        } else{
//                                                            printf("服务券向客户端发送失败了 %d\n");
//                                                        };
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
                                            send(client_fds[i],reply_str,strlen(reply_str),0);
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
                                                            if((send_le = send(client_fds[i],reply_str, strlen(reply_str),0)) > 0)
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
                                                            if((send_le = send(client_fds[i],reply_str, strlen(reply_str),0)) > 0)
                                                            {
                                                                printf("服务券向客户端发送大小为 %d\n",send_le);
                                                                bStop = true;
                                                                break;

                                                            } else{
                                                                printf("服务券向客户端发送失败了 %d\n");
                                                            };
                                                        } else if (mitus->second.compare("IDLE") == 0){
//                                                        char reply_str[50]="10";
//                                                        int send_le;
//                                                        if((send_le = send(newfd,reply_str, strlen(reply_str),0)) > 0)
//                                                        {
//                                                            printf("服务券向客户端发送大小为 %d\n",send_le);
//                                                        } else{
//                                                            printf("服务券向客户端发送失败了 %d\n");
//                                                        };
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
                                            send(client_fds[i],reply_str,strlen(reply_str),0);
                                        }
                                    }

                                }
                            };
                        }
                        else if(byte_num < 0)
                        {
                            printf("从客户端(%d)接受消息出错.\n", i);
                        }
                        else
                        {
                            FD_CLR(client_fds[i], &server_fd_set);
                            client_fds[i] = 0;
                            printf("客户端(%d)退出了\n", i);
                        }
                    }
                }
            }
        }
    }
    return 0;
}
