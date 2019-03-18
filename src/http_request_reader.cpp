#include "http_request_reader.hpp"
#include "get_request_processor.hpp"
#include "head_request_processor.hpp"
#include "unknown_request_processor.hpp"
#include "util.hpp"

#include <iostream>


HTTPRequestReader::~HTTPRequestReader() {
    stop();
}

void HTTPRequestReader::start() {
    if (!self) {
        self = shared_from_this();
        read_request();
    }
}

void HTTPRequestReader::stop() {
    if (self) {
        boost::system::error_code ec;
        socket_.cancel(ec);
        socket_.close(ec);
        HTTPRequestReaderPtr haron;
        haron.swap(self);
    }
}

void HTTPRequestReader::read_request() {
    if (!server_->is_running()) {
        stop();
        return;
    }

    boost::asio::async_read_until(socket_, boost::asio::dynamic_buffer(buffer_), CRLF CRLF,
        [self_sptr = shared_from_this()] (boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }
            else if (ec == boost::asio::error::eof && bytes_transferred == 0) {
                self_sptr->stop();
            }
            else if ((ec && ec != boost::asio::error::eof) || bytes_transferred < std::strlen(CRLF CRLF)) {
                if (ec) {
                    std::clog << "Failed to read http request: " << ec.message() << std::endl;
                }
                else if (bytes_transferred < std::strlen(CRLF CRLF)) {
                    std::clog << "Failed to read http request: \\r\\n\\r\\n expected on the connected socket" << std::endl;
                }
                self_sptr->stop();
            }
            else {
                try {
                    self_sptr->morph_into_request();
                }
                catch (const std::exception& ex) {
                    std::clog << "Failed to read http request: " << ex.what() << std::endl;
                    self_sptr->stop();
                }
            }
        }
    );
}

void HTTPRequestReader::morph_into_request() {
    std::string method;
    std::string uri;
    std::string version;
    std::map<std::string, std::string> headers;

    std::string line;
    do {
        line.clear();
        auto line_end = buffer_.find(CRLF);

        if (line_end == 0) {
            buffer_.erase(0, std::strlen(CRLF));
        }
        else if (line_end != std::string::npos) {
            line.assign(buffer_, 0, line_end);
            buffer_.erase(0, line_end + std::strlen(CRLF));

            if (method.empty()) {
                std::smatch match_results;

                if (!std::regex_match(line, match_results, request_line_rx_g)) {
                    throw std::runtime_error("bad HTTP request: invalid request line: " + make_printable(line));
                }

                method = match_results[1];
                uri = match_results[2];
                version = match_results[3];

                if (version != "HTTP/1.0" && version != "HTTP/1.1") {
                    throw std::runtime_error("bad HTTP request: invalid HTTP version: '" + version + "', only HTTP/1.0 and HTTP/1.1 are supported");
                }
            }
            else {
                std::smatch match_results;

                if (!std::regex_match(line, match_results, header_rx_g)) {
                    throw std::runtime_error("bad HTTP request: invalid header: " + make_printable(line));
                }

                headers[match_results[1]] = match_results[2];
            }
        }
    } while (line.empty());

    if (method.empty()) {
        throw std::runtime_error("bad HTTP request: no request line received");
    }

    if (method == "GET") {
        std::make_shared<GETRequestProcessor>(
            std::move(server_), std::move(socket_),
            std::move(uri), std::move(headers), std::move(buffer_)
        )->start();
    }
    else if (method == "HEAD") {
        std::make_shared<HEADRequestProcessor>(
            std::move(server_), std::move(socket_),
            std::move(uri), std::move(headers), std::move(buffer_)
        )->start();
    }
    else {
        std::make_shared<UNKNOWNRequestProcessor>(
            std::move(server_), std::move(socket_),
            std::move(uri), std::move(headers), std::move(buffer_)
        )->start();
    }
    stop();
}
