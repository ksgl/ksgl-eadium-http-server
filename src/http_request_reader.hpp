#pragma once

#include "server.hpp"
#include "connection.hpp"

#include <boost/asio.hpp>

#include <string>
#include <memory>


class HTTPRequestReader;
using HTTPRequestReaderPtr = std::shared_ptr<HTTPRequestReader>;

class HTTPRequestReader final
    : public Connection
    , public std::enable_shared_from_this<HTTPRequestReader>
{
public:
    using Connection::Connection;
    ~HTTPRequestReader();

    void start();
    void stop();

private:
    void read_request();
    void morph_into_request();

private:
    HTTPRequestReaderPtr self;
    std::string buffer_;
};

