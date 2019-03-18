#pragma once

#include "http_request_processor.hpp"

#include <memory>


class HEADRequestProcessor;
using HEADRequestProcessorPtr = std::shared_ptr<HEADRequestProcessor>;

class HEADRequestProcessor final
    : public HTTPRequestProcessor
    , public std::enable_shared_from_this<HEADRequestProcessor>
{
public:
    using HTTPRequestProcessor::HTTPRequestProcessor;

    ~HEADRequestProcessor();

    void start();
    void stop() noexcept;

private:
    void send_response();

private:
    HEADRequestProcessorPtr self;
};

