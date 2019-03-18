#pragma once

#include "connection.hpp"

#include <string>
#include <map>


class HTTPRequestProcessor
    : public Connection
{
public:
    HTTPRequestProcessor(
        ServerPtr&& server, boost::asio::ip::tcp::socket&& socket,
        std::string&& uri, std::map<std::string, std::string>&& headers, std::string&& body
    );

protected:
    ~HTTPRequestProcessor() = default;

protected:
    std::string uri_;
    std::map<std::string, std::string> headers_;
    std::string body_;
};
