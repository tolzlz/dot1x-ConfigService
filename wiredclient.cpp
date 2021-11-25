//
// Created by root on 2021/9/4.
//

#include "wiredclient.h"
#include <string>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
//#include <bsd/string.h>
#include <iostream>
#include <cstring>
#include <malloc.h>
#include <stdlib.h>
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)


//namespace wired{

    static std::string to_string(int val) {
        char *buf = NULL;
        int size;
        int temp;
        if (val < 0) {
            temp = -val;
            size = 2;
        }else{
            temp = val;
            size = 1;
        }
        for (; temp > 0; temp = temp / 10, size++);
        size++;
        buf = (char *)malloc(size);
        if (buf == NULL) {
            return "";
        }
        memset(buf, 0, size);
        sprintf(buf, "%d", val);
        std::string re(buf);
        free(buf);
        return re;
    }


    wiredclient::wiredclient(const std::string & wpa_control_path)
    {
        wired_context_ = nullptr;
        wpa_control_path_ = wpa_control_path;
        SetConnStatus(STEP_INIT);
        Init();

    }
    wiredclient::~wiredclient()
    {
        Close(wired_context_);
    }

    bool wiredclient::Init() {
        wired_context_ = Open(wpa_control_path_);
        if (wired_context_ == nullptr) {
            return false;
        }
        SetConnStatus(STEP_INIT);
        return true;
    }
    void wiredclient::Close(WiredContext * context) {
        if (context == nullptr)
            return;
        unlink(context->local.sun_path);
        if (context->s >= 0)
            close(context->s);
        free(context);
    }


    void wiredclient::SetConnStatus(ConnectStatus status) {
        step_ = status;
    }


    WiredContext * wiredclient::Open(const std::string & path) {
        struct WiredContext *ctrl;
        ctrl = (struct WiredContext*)malloc(sizeof(struct WiredContext));
        if (ctrl == nullptr) {
            return nullptr;
        }
        memset(ctrl, 0, sizeof(struct WiredContext));
        static int counter = 0;
        int ret;
        int tries = 0;
        size_t res;
        ctrl->s = socket(PF_UNIX, SOCK_DGRAM, 0);
        if (ctrl->s < 0) {
            free(ctrl);
            return nullptr;
        }
        ctrl->local.sun_family = AF_UNIX;
        counter++;
        try_again:
        ret = snprintf(ctrl->local.sun_path, sizeof(ctrl->local.sun_path),
                       "/data/local/tmp" "/"
                       "wpa_ctrl_" "%d-%d",
                       (int)getpid(), counter);
        if (ret < 0 || (size_t)ret >= sizeof(ctrl->local.sun_path)) {
            close(ctrl->s);
            free(ctrl);
            return nullptr;
        }
        tries++;
        if (bind(ctrl->s, (struct sockaddr *) &ctrl->local,
                 sizeof(ctrl->local)) < 0) {
            if (errno == EADDRINUSE && tries < 2) {
                /*
                * getpid() returns unique identifier for this instance
                * of wpa_ctrl, so the existing socket file must have
                * been left by unclean termination of an earlier run.
                * Remove the file and try again.
                */
                unlink(ctrl->local.sun_path);
                goto try_again;
            }
            close(ctrl->s);
            free(ctrl);
            return nullptr;
        }
        ctrl->dest.sun_family = AF_UNIX;
//        res = strlcpy(ctrl->dest.sun_path, wpa_control_path_.data(),
//                      sizeof(ctrl->dest.sun_path));

        strncpy(ctrl->dest.sun_path, wpa_control_path_.data(),sizeof(ctrl->dest.sun_path));

        int i = 0;
        while (ctrl->dest.sun_path[i] !='\0')
        {
            i++;
        }
        res = i;

        if (res >= sizeof(ctrl->dest.sun_path)) {
            close(ctrl->s);
            free(ctrl);
            return nullptr;
        }
        if (connect(ctrl->s, (struct sockaddr *) &ctrl->dest,
                    sizeof(ctrl->dest)) < 0) {
            close(ctrl->s);
            unlink(ctrl->local.sun_path);
            free(ctrl);
            return nullptr;
        }
        return ctrl;
    }

    bool wiredclient::Request(WiredContext * context, const std::string & cmd, std::string& reply) {
        int res;
        fd_set rfds;
        struct timeval tv;
        const char *_cmd;
        char *cmd_buf = NULL;
        size_t _cmd_len;
        _cmd = cmd.data();
        _cmd_len = cmd.length();
        if (wired_context_ == nullptr) {
            return false;
        }
        if (send(wired_context_->s, _cmd, _cmd_len, 0) < 0) {
            free(cmd_buf);
            return -1;
        }
        free(cmd_buf);
        for (;;) {
            tv.tv_sec = 10;
            tv.tv_usec = 0;
            FD_ZERO(&rfds);
            FD_SET(wired_context_->s, &rfds);
            res = select(wired_context_->s + 1, &rfds, NULL, NULL, &tv);
            if (res < 0)
                return false;
            if (FD_ISSET(wired_context_->s, &rfds)) {
                char temp[1024] = {0};
                int temp_len = 1024;
                res = recv(wired_context_->s, temp, temp_len, 0);
                if (res < 0)
                    return false;
                if (res > 0 && temp[0] == '<') {
                    continue;
                }
                reply = temp;
                break;
            } else {
                return false;
            }
        }
        return true;
    }
    bool wiredclient::CheckCommandWithOk(const std::string cmd) {
        std::string recv;
        if (!Request(wired_context_, cmd, recv)) {
            return false;
        }
        if (strstr(recv.data(), "OK") == NULL) {
            return false;
        }
        return true;
    }



    bool wiredclient::CleanAllWired() {
        CheckCommandWithOk("REMOVE_NETWORK all");
        CheckCommandWithOk("DISABLE_NETWORK all");
        return true;
    }
    bool wiredclient::AddWired(int & id) {
        std::string add_cmd = "ADD_NETWORK";
        std::string recv;
        if (!Request(wired_context_, add_cmd, recv)) {
            return false;
        }
        id = atoi(recv.data());
        return true;
    }

    bool wiredclient::SetProtocol(int id, int en_crypt) {
        std::string cmd = "SET_NETWORK " + to_string(id);
        if (en_crypt) {
            cmd += " key_mgmt IEEE8021X";
            return CheckCommandWithOk(cmd);
        } else {
            cmd += " key_mgmt NONE";
            return CheckCommandWithOk(cmd);
        }
    }
    //1 md5; 2,peap; 3,tls
    bool wiredclient::SetEapType(int id,const std::string &eap) {
        std::string cmd = "SET_NETWORK " + to_string(id) + " eap " + eap;
        return CheckCommandWithOk(cmd);
    }

    bool wiredclient::SetPassword(const std::string & password, int id) {
        std::string cmd = "SET_NETWORK " + to_string(id) + " password " + "\"" + password + "\"";
        return CheckCommandWithOk(cmd);
    }
    bool wiredclient::SetIdentity(const std::string & identity,int id){
        std::string  cmd = "SET_NETWORK "+ to_string(id) + " identity " + "\"" + identity + "\"";
        return CheckCommandWithOk(cmd);
    }
//    bool wiredclient::SetEapolFlags(int id,int flags)
//    {
//        std::string cmd = "SET_NETWORK " + to_string(id) + " eapol_flags " + std::to_string(flags);
//        return CheckCommandWithOk(cmd);
//    }


    bool wiredclient::SetCaCert(int id,const std::string& ca_cert)
    {
        std::string  cmd = "SET_NETWORK "+ to_string(id) + " ca_cert " + "\"" + ca_cert + "\"";
        return CheckCommandWithOk(cmd);
    }
    bool wiredclient::SetClientCert(int id,const std::string& client_cert){
        std::string  cmd = "SET_NETWORK "+ to_string(id) + " client_cert " + "\"" + client_cert + "\"";
        return CheckCommandWithOk(cmd);
    }
    bool wiredclient::SetPrivateKey(int id,const std::string& private_key){
        std::string  cmd = "SET_NETWORK "+ to_string(id) + " private_key " + "\"" + private_key + "\"";
        return CheckCommandWithOk(cmd);
    }
    bool wiredclient::SetPrivateKeyPassword(int id,const std::string& private_key_passwd)
    {
        std::string  cmd = "SET_NETWORK "+ to_string(id) + " private_key_passwd " + "\"" + private_key_passwd + "\"";
        return CheckCommandWithOk(cmd);
    }

    bool wiredclient::SetPhase1(int id,const std::string& phase1)
    {
        std::string std1 = "peaplabel=1";
        std::string  cmd = "SET_NETWORK "+ to_string(id) + " phase1 " + "\""+std1+"\"";
        return CheckCommandWithOk(cmd);
    }
    bool wiredclient::SetPhase2(int id,const std::string& phase2)
    {
        std::string  std2 = "auth=MSCHAPV2";
        std::string  cmd = "SET_NETWORK "+ to_string(id) + " phase2 " + "\""+std2+"\"";
        return CheckCommandWithOk(cmd);
    }

    bool wiredclient::SetPriority(int id,const std::string& priority){
        std::string cmd = "SET_NETWORK " + to_string(id) + " priority " + priority;
        return CheckCommandWithOk(cmd);
    }
    bool wiredclient::SetAnonymousIdentity(int id,const std::string& anonymous_identity){
        std::string  cmd = "SET_NETWORK "+ to_string(id) + " anonymous_identity " + "\"" + anonymous_identity + "\"";
        return CheckCommandWithOk(cmd);
    }








    bool wiredclient::EnableWired(int id) {
        std::string cmd = "ENABLE_NETWORK " + to_string(id);
        return CheckCommandWithOk(cmd);
    }

    bool wiredclient::GetConnectStatus(std::string& recv_str) {
        std::string cmd = "STATUS";
        std::string recv;
        int addr;
        switch (step_) {
            case STEP_INIT:
                if (!Request(wired_context_, cmd, recv)) {
                    return false;
                }
                //write to buf;
                //std::cout<<"recv str :"<<recv<<std::endl;
                recv_str = recv;

        }
        return false;
    }


    bool wiredclient::ConnectWiredWithMD5(const std::string &identity, const std::string &password) {
        int net_id;
        SetConnStatus(STEP_INIT);
        if (!CleanAllWired()) {
            return false;
        }
        if (!AddWired(net_id)) {
            return false;
        }
        if (!SetProtocol(net_id, 1)) {
            return false;
        }
        if (!SetIdentity(identity, net_id)) {
            return false;
        }
        if (!SetPassword(password, net_id)) {
            return false;
        }

        if (!SetEapType(net_id,"MD5"))
        {
            return false;
        }
//        if (!SetEapolFlags(net_id,0)){
//            return false;
//        }
        if (!EnableWired(net_id)) {
            return false;
        }
        return true;

    }




    bool wiredclient::ConnectWiredWithTTLS(const std::string &identity, const std::string &anonymous_identity,
                                           const std::string &password, const std::string &ca_cert) {
        int net_id;
        SetConnStatus(STEP_INIT);
        if (!CleanAllWired()) {
            return false;
        }
        if (!AddWired(net_id)) {
            return false;
        }
        //key_mgmt
        if (!SetProtocol(net_id, 1)) {
            return false;
        }
        //identity
        if (!SetIdentity(identity, net_id)) {
            return false;
        }
        //password
        if (!SetPassword(password, net_id)) {
            return false;
        }
        //eap
        if (!SetEapType(net_id,"TTLS"))
        {
            return false;
        }
        //ca_cert
        if (!SetCaCert(net_id,ca_cert))
        {
            return false;
        }
        //anonymous_identity
        if (!SetAnonymousIdentity(net_id,anonymous_identity))
        {
            return false;
        }
        if (!EnableWired(net_id)) {
            return false;
        }
        return true;
    }

    bool wiredclient::ConnectWiredWithPEAP(const std::string &identity, const std::string &password,
                                           const std::string &ca_cert) {
        int net_id;
        SetConnStatus(STEP_INIT);
        if (!CleanAllWired()) {
            return false;
        }
        if (!AddWired(net_id)) {
            return false;
        }
        //key_mgmt
        if (!SetProtocol(net_id, 1)) {
            return false;
        }
        //identity
        if (!SetIdentity(identity, net_id)) {
            return false;
        }
        //password
        if (!SetPassword(password, net_id)) {
            return false;
        }
        //eap
        if (!SetEapType(net_id,"PEAP"))
        {
            return false;
        }
        //ca_cert
        if (!SetCaCert(net_id,ca_cert))
        {
            return false;
        }
        //phase1
        if (!SetPhase1(net_id,""))
        {
            return false;
        }
        //phase2
        if (!SetPhase2(net_id,""))
        {
            return false;
        }
        if (!EnableWired(net_id)) {
            return false;
        }
        return true;
    }

    bool wiredclient::ConnectWiredWithTLS(const std::string &identity, const std::string &ca_cert,
                                          const std::string &client_cert, const std::string &private_key,
                                          const std::string &private_key_passwd) {
        int net_id;
        SetConnStatus(STEP_INIT);
        if (!CleanAllWired()) {
            return false;
        }
        if (!AddWired(net_id)) {
            return false;
        }
        //key_mgmt
        if (!SetProtocol(net_id, 1)) {
            return false;
        }
        //identity
        if (!SetIdentity(identity, net_id)) {
            return false;
        }
        //eap
        if (!SetEapType(net_id,"TLS"))
        {
            return false;
        }
        //ca_cert
        if (!SetCaCert(net_id,ca_cert))
        {
            return false;
        }
        //client_cert
        if (!SetClientCert(net_id,client_cert))
        {
            return false;
        }
        //private_key
        if (!SetPrivateKey(net_id,private_key))
        {
            return false;
        }
        //private_key_passwd
        if (!SetPrivateKeyPassword(net_id,private_key_passwd))
        {
            return false;
        }
        if (!EnableWired(net_id)) {
            return false;
        }
        return true;
    }
//}