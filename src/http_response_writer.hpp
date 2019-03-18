#pragma once

#include "connection.hpp"

#include <atomic>
#include <memory>
#include <map>
#include <string>


class HTTPBufferResponseWriter;
using HTTPBufferResponseWriterPtr = std::shared_ptr<HTTPBufferResponseWriter>;

class HTTPBufferResponseWriter final
    : public Connection
    , public std::enable_shared_from_this<HTTPBufferResponseWriter>
{
public:
    HTTPBufferResponseWriter(
        ServerPtr&& server, boost::asio::ip::tcp::socket&& socket,
        int code, std::map<std::string, std::string>&& headers, std::string&& body
    );

    ~HTTPBufferResponseWriter();

    void start();
    void stop() noexcept;
    
private:
    void send_complete_response();
    void morph_into_request_reader();

private:
    HTTPBufferResponseWriterPtr self;
    const bool close_;
    std::string buffer_;
};

class HTTPStreamResponseWriter;
using HTTPStreamResponseWriterPtr = std::shared_ptr<HTTPStreamResponseWriter>;

class HTTPStreamResponseWriter final
    : public Connection
    , public std::enable_shared_from_this<HTTPStreamResponseWriter>
{
public:
    HTTPStreamResponseWriter(
        ServerPtr&& server, boost::asio::ip::tcp::socket&& socket,
        int code, std::map<std::string, std::string>&& headers, boost::asio::posix::stream_descriptor&& stream
    );

    ~HTTPStreamResponseWriter();

    void start();
    void stop() noexcept;
    
private:
    void fill_in_buffer();
    void flush_out_buffer();
    void sync_streams();
    void morph_into_request_reader();

private:
    HTTPStreamResponseWriterPtr self;
    boost::asio::posix::stream_descriptor stream_;
    const bool close_;
    std::atomic<std::size_t> data_to_send_{0};

    std::unique_ptr<boost::asio::strand<boost::asio::io_context::executor_type>> strand_;

    std::string in_buffer_;
    std::atomic<bool> in_closed_{false};
    std::atomic<bool> in_overflown_{false};

    std::string out_buffer_;
    std::atomic<bool> out_closed_{false};
    std::atomic<bool> out_starving_{false};

    static constexpr std::size_t max_in_buffer_size_ = 1 << 20; // 1 MB
};

