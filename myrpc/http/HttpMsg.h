/* 封装了http的请求和响应.
 * 定义了HttpMessage, HttpRequest和HttpResponse.
 *  */

#pragma once

#include "../msg.h"
#include <vector>
#include <string>

namespace myrpc {

//Http的基本消息格式
class HttpMessage : virtual public BaseMessage {
public:
    enum class Direction {
        NONE = 0,
        REQUEST,
        RESPONSE,
        MAX,
    };

    static const char *HEADER_CONTENT_LENGTH;
    static const char *HEADER_CONTENT_TYPE;
    static const char *HEADER_CONNECTION;
    static const char *HEADER_PROXY_CONNECTION;
    static const char *HEADER_TRANSFER_ENCODING;
    static const char *HEADER_DATE;
    static const char *HEADER_SERVER;

    static const char *HEADER_X_MYRPC_RESULT;

    HttpMessage() = default;
    virtual ~HttpMessage() override = default;

    virtual int ToPb(google::protobuf::Message *const message) const override;
    virtual int FromPb(const google::protobuf::Message &message) override;
    virtual size_t size() const override;

    void AddHeader(const char *name, const char *value);
    void AddHeader(const char *name, int value);
    bool RemoveHeader(const char *name);
    size_t GetHeaderCount() const;
    const char *GetHeaderName(size_t index) const;
    const char *GetHeaderValue(size_t index) const;
    const char *GetHeaderValue(const char *name) const;
    void AppendContent(const void *content, const int length = 0, const int max_length = 0);

    const std::string &content() const;
    void set_content(const char *const content, const int length = 0);
    std::string *mutable_content();

    const char *version() const;
    void set_version(const char *version);

    Direction direction() const { 
        return direction_; 
    }

protected:
    void set_direction(const Direction direction)  {
        direction_ = direction;
    }

    std::vector<std::string> header_name_list_, header_value_list_;

private:
    std::string content_;
    char version_[16];
    Direction direction_{Direction::NONE};
};

//Http的request
class HttpRequest : public HttpMessage, public BaseRequest {
public:
    HttpRequest();
    virtual ~HttpRequest() override;

    virtual int Send(BaseTcpStream &socket) const override;

    virtual BaseResponse *GenResponse() const override;
    virtual bool keep_alive() const override;
    virtual void set_keep_alive(const bool keep_alive) override;

    void AddParam(const char *name, const char *value);
    bool RemoveParam(const char *name);
    size_t GetParamCount() const;
    const char *GetParamName(size_t index) const;
    const char *GetParamValue(size_t index) const;
    const char *GetParamValue(const char *name) const;

    int IsMethod(const char *method) const;

    const char *method() const;
    void set_method(const char *method);

private:
    char method_[16];
    std::vector<std::string> param_name_list_, param_value_list_;
};

//Http的response
class HttpResponse : public HttpMessage, public BaseResponse {
public:
    HttpResponse();
    virtual ~HttpResponse() override;

    virtual int Send(BaseTcpStream &socket) const override;

    virtual void SetFake(FakeReason reason) override;

    virtual int Modify(const bool keep_alive, const std::string &version) override;

    virtual int result() override;
    virtual void set_result(const int result) override;

    void set_status_code(int status_code);
    int status_code() const;

    void set_reason_phrase(const char *reason_phrase);
    const char *reason_phrase() const;

private:
    int status_code_;
    char reason_phrase_[128];
};

}