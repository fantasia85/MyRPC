/* Http消息处理类，继承自BaseMessageHandler类. */

#include "HttpMsgHandler.h"
#include "HttpMsg.h"
#include "HttpProtocol.h"
#include "../network/SocketStreamBase.h"

namespace myrpc {

int HttpMessageHandler::RecvRequest(BaseTcpStream &socket, BaseRequest *&req) {
    HttpRequest *http_req = new HttpRequest;

    int ret = HttpProtocol::RecvReq(socket, http_req);
    if (ret == 0) {
        req_ = req = http_req;
        version_ = (http_req->version() != nullptr ? http_req->version() : "");
        keep_alive_ = http_req->keep_alive();
    }
    else {
        delete http_req;
        http_req = nullptr;
    }

    return  ret;
}

int HttpMessageHandler::RecvResponse(BaseTcpStream &socket, BaseResponse *&resp) {
    HttpResponse *http_resp = new HttpResponse;

    int ret = HttpProtocol::RecvResp(socket, http_resp);
    if (ret == 0) 
        resp = http_resp;
    else {
        delete http_resp;
        http_resp = nullptr;
    }

    return ret;
}

int HttpMessageHandler::GenRequest(BaseRequest *&req) {
    req = new HttpRequest;

    return 0;
}

int HttpMessageHandler::GenResponse(BaseResponse *&resp) {
    resp = req_->GenResponse();
    resp->Modify(keep_alive_, version_);

    return 0;
}

bool HttpMessageHandler::keep_alive() const {
    return keep_alive_;
}

}