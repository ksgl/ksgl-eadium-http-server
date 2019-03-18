#include "unknown_request_processor.hpp"
#include "http_response_writer.hpp"
#include "util.hpp"


UNKNOWNRequestProcessor::~UNKNOWNRequestProcessor() {
    stop();
}

void UNKNOWNRequestProcessor::start() {
    if (!self) {
        self = shared_from_this();
        send_response();
    }
}

void UNKNOWNRequestProcessor::stop() noexcept {
    if (self) {
        boost::system::error_code ec;
        socket_.cancel(ec);
        socket_.close(ec);
        UNKNOWNRequestProcessorPtr haron;
        haron.swap(self);
    }
}

void UNKNOWNRequestProcessor::send_response() {
    const int code = 405;
    std::map<std::string, std::string> headers;

    headers["Server"] = server_->get_server_string();
    headers["Connection"] = "close";
    headers["Date"] = rfc_1123_now();

    std::make_shared<HTTPBufferResponseWriter>(
        std::move(server_), std::move(socket_),
        code, std::move(headers), ""
    )->start();

    stop();
}

