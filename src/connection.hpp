#pragma once

#include "server.hpp"

#include <mutex>

#include <boost/asio.hpp>


class Connection {
public:
    Connection(ServerPtr&& server, boost::asio::ip::tcp::socket&& socket);

protected:
    ~Connection() = default;

protected:
    ServerPtr server_;
    std::recursive_mutex mutex_;
    boost::asio::ip::tcp::socket socket_;
};

