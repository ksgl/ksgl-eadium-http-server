#pragma once

#include "http_request_processor.hpp"

#include <memory>


class GETRequestProcessor;
using GETRequestProcessorPtr = std::shared_ptr<GETRequestProcessor>;

class GETRequestProcessor final
    : public HTTPRequestProcessor
    , public std::enable_shared_from_this<GETRequestProcessor>
{
public:
    using HTTPRequestProcessor::HTTPRequestProcessor;

    ~GETRequestProcessor();

    void start();
    void stop() noexcept;

private:
    void send_response();

private:
    GETRequestProcessorPtr self;
};

