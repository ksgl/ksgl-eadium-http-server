#pragma once

#include "http_request_processor.hpp"

#include <memory>


class UNKNOWNRequestProcessor;
using UNKNOWNRequestProcessorPtr = std::shared_ptr<UNKNOWNRequestProcessor>;

class UNKNOWNRequestProcessor final
    : public HTTPRequestProcessor
    , public std::enable_shared_from_this<UNKNOWNRequestProcessor>
{
public:
    using HTTPRequestProcessor::HTTPRequestProcessor;

    ~UNKNOWNRequestProcessor();

    void start();
    void stop() noexcept;

private:
    void send_response();

private:
    UNKNOWNRequestProcessorPtr self;
};

