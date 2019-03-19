#include "http_response_writer.hpp"
#include "http_request_reader.hpp"
#include "util.hpp"

#include <boost/lexical_cast.hpp>

#include <iostream>


HTTPBufferResponseWriter::HTTPBufferResponseWriter(
    ServerPtr&& server, boost::asio::ip::tcp::socket&& socket,
    int code, std::map<std::string, std::string>&& headers, std::string&& body
)
    : Connection(std::forward<decltype(server)>(server), std::forward<decltype(socket)>(socket))
    , close_(headers["Connection"] == "close")
{
    buffer_ = "HTTP/1.1 " + std::to_string(code) + " " + code_text(code) + CRLF;
    for (auto& header : headers) {
        buffer_ += header.first + ": " + header.second + CRLF;
    }
    buffer_ += CRLF + body;
}

HTTPBufferResponseWriter::~HTTPBufferResponseWriter() {
    stop();
}

void HTTPBufferResponseWriter::start() {
    if (!self) {
        self = shared_from_this();
        send_complete_response();
    }
}

void HTTPBufferResponseWriter::stop() noexcept {
    if (self) {
        boost::system::error_code ec;
        socket_.cancel(ec);
        socket_.close(ec);
        HTTPBufferResponseWriterPtr haron;
        haron.swap(self);
    }
}

void HTTPBufferResponseWriter::send_complete_response() {
    boost::asio::async_write(socket_, boost::asio::buffer(buffer_),
        [self_sptr = shared_from_this(), this] (boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }
            else if (ec) {
                std::clog << "Failed to write http response: " << ec.message() << std::endl;
                stop();
            }
            else {
                if (!close_) {
                    try {
                        morph_into_request_reader();
                    }
                    catch (const std::exception& ex) {
                        std::clog << "Failed write http response: " << ex.what() << std::endl;
                        stop();
                    }
                }
                else {
                    stop();
                }
            }
        }
    );
}

void HTTPBufferResponseWriter::morph_into_request_reader() {
    std::make_shared<HTTPRequestReader>(std::move(server_), std::move(socket_))->start();
    stop();
}

HTTPStreamResponseWriter::HTTPStreamResponseWriter(
    ServerPtr&& server, boost::asio::ip::tcp::socket&& socket,
    int code, std::map<std::string, std::string>&& headers, boost::asio::posix::stream_descriptor&& stream
)
    : Connection(std::forward<decltype(server)>(server), std::forward<decltype(socket)>(socket))
    , stream_(std::forward<decltype(stream)>(stream))
    , close_(headers["Connection"] == "close")
{
    strand_ = std::make_unique<boost::asio::strand<boost::asio::io_context::executor_type>>(socket_.get_executor());

    try {
        data_to_send_ = boost::lexical_cast<decltype(data_to_send_)>(headers["Content-Length"]);
    }
    catch (const boost::bad_lexical_cast&) {
    }

    out_buffer_ = "HTTP/1.1 " + std::to_string(code) + " " + code_text(code) + CRLF;
    for (auto& header : headers) {
        out_buffer_ += header.first + ": " + header.second + CRLF;
    }
    out_buffer_ += CRLF;

    data_to_send_ += out_buffer_.size();
}

HTTPStreamResponseWriter::~HTTPStreamResponseWriter() {
    stop();
}

void HTTPStreamResponseWriter::start() {
    if (!self) {
        self = shared_from_this();
        fill_in_buffer();
        flush_out_buffer();
    }
}

void HTTPStreamResponseWriter::stop() noexcept {
    if (self) {
        boost::system::error_code ec;
        socket_.cancel(ec);
        socket_.close(ec);
        stream_.cancel(ec);
        stream_.close(ec);
        HTTPStreamResponseWriterPtr haron;
        haron.swap(self);
    }
}

void HTTPStreamResponseWriter::fill_in_buffer() {
    in_buffer_ready_ = false;
    boost::asio::async_read(stream_, boost::asio::dynamic_buffer(in_buffer_, HTTPStreamResponseWriter::max_in_buffer_size_),
        [self_sptr = shared_from_this(), this] (boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec == boost::asio::error::operation_aborted) {
                in_closed_ = true;
                in_buffer_ready_ = true;
            }
            else if (ec) {
                if (ec != boost::asio::error::eof) {
                    std::clog << "Failed to prepare http response: " << ec.message() << std::endl;
                }
                in_closed_ = true;
                in_buffer_ready_ = true;
                sync_streams();
            }
            else if (!server_->is_running() || out_closed_) {
                in_closed_ = true;
                in_buffer_ready_ = true;
                sync_streams();
            }
            else {
                if (in_buffer_.size() >= HTTPStreamResponseWriter::max_in_buffer_size_ || out_buffer_ready_) {
                    in_buffer_ready_ = true;
                    sync_streams();
                }
                else {
                    fill_in_buffer();
                }
            }
        }
    );
}

void HTTPStreamResponseWriter::flush_out_buffer() {
    out_buffer_ready_ = false;
    boost::asio::async_write(socket_, boost::asio::dynamic_buffer(out_buffer_),
        [self_sptr = shared_from_this(), this] (boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec == boost::asio::error::operation_aborted) {
                out_closed_ = true;
                out_buffer_ready_ = true;
            }
            else if (ec) {
                std::clog << "Failed to write http response: " << ec.message() << std::endl;
                out_closed_ = true;
                out_buffer_ready_ = true;
                sync_streams();
            }
            else if (!server_->is_running()) {
                out_closed_ = true;
                out_buffer_ready_ = true;
                sync_streams();
            }
            else {
                if (bytes_transferred > data_to_send_) {
                    data_to_send_ = 0;
                }
                else {
                    data_to_send_ -= bytes_transferred;
                }

                if (data_to_send_ == 0) {
                    out_closed_ = true;
                    out_buffer_ready_ = true;
                    sync_streams();
                }
                else if (out_buffer_.empty()) {
                    out_buffer_ready_ = true;
                    sync_streams();
                }
                else {
                    flush_out_buffer();
                }
            }
        }
    );
}

void HTTPStreamResponseWriter::sync_streams() {
    boost::asio::post(*strand_,
        [self_sptr = shared_from_this(), this] () {
            if (in_closed_ && out_closed_) {
                if (data_to_send_ == 0 && !close_) {
                    try {
                        morph_into_request_reader();
                    }
                    catch (const std::exception& ex) {
                        std::clog << "Failed write http response: " << ex.what() << std::endl;
                        stop();
                    }
                }
                else {
                    stop();
                }
            }
            else if (
                     (!out_closed_ && out_buffer_ready_) &&
                     (in_closed_ || in_buffer_ready_) &&
                     out_buffer_.empty() && !in_buffer_.empty()
            ) {
                out_buffer_.swap(in_buffer_);

                flush_out_buffer();

                if (!in_closed_) {
                    fill_in_buffer();
                }
            }
        }
    );
}

void HTTPStreamResponseWriter::morph_into_request_reader() {
    std::make_shared<HTTPRequestReader>(std::move(server_), std::move(socket_))->start();
    stop();
}

