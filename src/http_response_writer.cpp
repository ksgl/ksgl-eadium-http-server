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
        [self_sptr = shared_from_this()] (boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }
            else if (ec) {
                std::clog << "Failed to write http response: " << ec.message() << std::endl;
                self_sptr->stop();
            }
            else {
                if (!self_sptr->close_) {
                    try {
                        self_sptr->morph_into_request_reader();
                    }
                    catch (const std::exception& ex) {
                        std::clog << "Failed write http response: " << ex.what() << std::endl;
                        self_sptr->stop();
                    }
                }
                else {
                    self_sptr->stop();
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
        data_to_send_ = boost::lexical_cast<std::size_t>(headers["Content-Length"]);
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
    std::scoped_lock lock(mutex_);
    if (!self) {
        self = shared_from_this();
        fill_in_buffer();
        flush_out_buffer();
    }
}

void HTTPStreamResponseWriter::stop() noexcept {
    std::scoped_lock lock(mutex_);
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
    in_overflown_ = false;
    boost::asio::async_read(stream_, boost::asio::dynamic_buffer(in_buffer_, HTTPStreamResponseWriter::max_in_buffer_size_),
        [self_sptr = shared_from_this()] (boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }
            else if (ec) {
                if (ec != boost::asio::error::eof) {
                    std::clog << "Failed to prepare http response: " << ec.message() << std::endl;
                }
                self_sptr->in_closed_ = true;
                self_sptr->sync_streams();
            }
            else if (!self_sptr->server_->is_running() || self_sptr->out_closed_) {
                self_sptr->in_closed_ = true;
                self_sptr->sync_streams();
            }
            else {
                if (self_sptr->out_starving_ || self_sptr->in_buffer_.size() >= HTTPStreamResponseWriter::max_in_buffer_size_) {
                    self_sptr->in_overflown_ = true;
                    self_sptr->sync_streams();
                }
                else {
                    self_sptr->fill_in_buffer();
                }
            }
        }
    );
}

void HTTPStreamResponseWriter::flush_out_buffer() {
    out_starving_ = false;
    boost::asio::async_write(socket_, boost::asio::dynamic_buffer(out_buffer_),
        [self_sptr = shared_from_this()] (boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }
            else if (ec) {
                std::clog << "Failed to write http response: " << ec.message() << std::endl;
                self_sptr->out_closed_ = true;
                self_sptr->sync_streams();
            }
            else if (!self_sptr->server_->is_running()) {
                self_sptr->out_closed_ = true;
                self_sptr->sync_streams();
            }
            else {
                if (bytes_transferred > self_sptr->data_to_send_) {
                    self_sptr->data_to_send_ = 0;
                }
                else {
                    self_sptr->data_to_send_ -= bytes_transferred;
                }

                if (self_sptr->data_to_send_ == 0) {
                    self_sptr->out_closed_ = true;
                    self_sptr->sync_streams();
                }
                else if (self_sptr->out_buffer_.empty()) {
                    self_sptr->out_starving_ = true;
                    self_sptr->sync_streams();
                }
                else {
                    self_sptr->flush_out_buffer();
                }
            }
        }
    );
}

void HTTPStreamResponseWriter::sync_streams() {
    boost::asio::post(*strand_,
        [self_sptr = shared_from_this()] () {
            if (self_sptr->in_closed_ && self_sptr->out_closed_) {
                if (self_sptr->data_to_send_ == 0 && !self_sptr->close_) {
                    try {
                        self_sptr->morph_into_request_reader();
                    }
                    catch (const std::exception& ex) {
                        std::clog << "Failed write http response: " << ex.what() << std::endl;
                        self_sptr->stop();
                    }
                }
                else {
                    self_sptr->stop();
                }
            }
            else if (
                     (!self_sptr->out_closed_ && self_sptr->out_starving_) &&
                     (self_sptr->in_overflown_ || self_sptr->in_closed_) &&
                     self_sptr->out_buffer_.empty() && !self_sptr->in_buffer_.empty()
            ) {
                self_sptr->out_buffer_.swap(self_sptr->in_buffer_);

                self_sptr->flush_out_buffer();

                if (!self_sptr->in_closed_) {
                    self_sptr->fill_in_buffer();
                }
            }
        }
    );
}

void HTTPStreamResponseWriter::morph_into_request_reader() {
    std::make_shared<HTTPRequestReader>(std::move(server_), std::move(socket_))->start();
    stop();
}

