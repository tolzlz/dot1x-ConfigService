//
// Created by root on 2021/9/4.
//

#ifndef BASEAUTH_WPA_WIREDCLIENT_H
#define BASEAUTH_WPA_WIREDCLIENT_H

#include <string>
#include <sys/un.h>

//namespace wired{
    //const std::string WPA_PATH = "/tmp/wpa_supplicant/eth0";  //进程间通信地址加上网络接口额名称
    const std::string WPA_PATH = "/data/local/tmp/eth0";  //进程间通信地址加上网络接口额名称
    struct WiredContext {
        int s;
        struct sockaddr_un local;
        struct sockaddr_un dest;
    };
    enum ConnectStatus {
//        STEP_READY = 0, //ready
//        STEP_CONNECT_AP_OK,
//        STEP_DHCP_IP, //
//        STEP_SUCCESS
        STEP_INIT,
    };
    class wiredclient {

    public:

        wiredclient(const std::string& wpa_control_path = WPA_PATH);
        ~wiredclient();
        bool GetInitStatus(){return wired_context_!=nullptr;}   //获取wpa进程间通信是否建立连接

        bool ConnectWiredWithMD5(const std::string &identity, const std::string &password);

        bool ConnectWiredWithTTLS(const std::string &identity, const std::string &anonymous_identity,
                                  const std::string &password, const std::string &ca_cert);

        bool ConnectWiredWithPEAP(const std::string &identity, const std::string &password, const std::string &ca_cert);

        bool
        ConnectWiredWithTLS(const std::string &identity, const std::string &ca_cert, const std::string &client_cert,
                            const std::string &private_key, const std::string &private_key_passwd);


        bool GetConnectStatus(std::string& recv_str);

    protected:
        bool Init();
        struct WiredContext* Open(const std::string& path);
        void Close(struct WiredContext* context);
        bool Request(struct WiredContext* context, const std::string& cmd,std::string& reply);
        bool CheckCommandWithOk(const std::string cmd);
        bool AddWired(int& id);
        bool SetEapType(int id,const std::string &eap);
        bool SetEapolFlags(int id,int flags);
        bool SetIdentity(const std::string& identity,int id);
        bool SetPassword(const std::string& password,int id);
        bool SetProtocol(int id, int en_crypt);

        bool SetCaCert(int id,const std::string& ca_cert);
        bool SetClientCert(int id,const std::string& client_cert);
        bool SetPrivateKey(int id,const std::string& private_key);
        bool SetPrivateKeyPassword(int id,const std::string& private_key_password);

        bool SetPhase1(int id,const std::string& phase1);
        bool SetPhase2(int id,const std::string& phase2);

        bool SetPriority(int id,const std::string& priority);
        bool SetAnonymousIdentity(int id,const std::string& anonymous_identity);


        bool CleanAllWired();
        bool EnableWired(int id);
        void SetConnStatus(ConnectStatus status);
        //获取wifi连接状态
    protected:
        std::string wpa_control_path_;
        struct WiredContext* wired_context_;
    private:
        int step_;
    };

//}
#endif //BASEAUTH_WPA_WIREDCLIENT_H
