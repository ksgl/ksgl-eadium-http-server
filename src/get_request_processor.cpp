#include "get_request_processor.hpp"
#include "http_response_writer.hpp"
#include "util.hpp"

#include <boost/filesystem.hpp>


GETRequestProcessor::~GETRequestProcessor() {
    stop();
}

void GETRequestProcessor::start() {
    if (!self) {
        self = shared_from_this();
        send_response();
    }
}

void GETRequestProcessor::stop() noexcept {
    if (self) {
        boost::system::error_code ec;
        socket_.cancel(ec);
        socket_.close(ec);
        GETRequestProcessorPtr haron;
        haron.swap(self);
    }
}

void GETRequestProcessor::send_response() {
    int code = 400;
    std::map<std::string, std::string> headers;

    headers["Server"] = server_->get_server_string();
    headers["Connection"] = "close";
    headers["Date"] = rfc_1123_now();
    
    int handle = -1;

    const auto file_path = remove_request_part(uri_);
    std::string decoded_file_path;
    if (url_decode(file_path, decoded_file_path)) {
        try {
            auto abs_doc_root = boost::filesystem::canonical(server_->get_doc_root());
            auto path = abs_doc_root / decoded_file_path;
            
            if (!boost::filesystem::exists(path)) {
                code = 404;
            }
            else {
                path = boost::filesystem::canonical(path);
                if (!is_in(path, abs_doc_root)) {
                    code = 400;
                }
                else {
                    if (boost::filesystem::is_directory(path)) {
                        path = path / server_->get_index_file_name();
                    }
                    
                    if (!boost::filesystem::exists(path)) {
                        code = 403; // index file doesn't exist -- 403
                    }
                    else {
                        auto status = boost::filesystem::status(path);
                        if (!boost::filesystem::is_regular(status)) {
                            code = 400;
                        }
                        else if (0 == (
                            status.permissions() &
                            (boost::filesystem::owner_read | boost::filesystem::group_read | boost::filesystem::others_read)
                        )) {
                            code = 403;
                        }
                        else {
                            handle = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
                            if (handle < 0) {
                                code = 400;
                            }
                            else {
                                code = 200;
                                if (headers_["Connection"] == "keep-alive") {
                                    headers["Connection"] = "keep-alive";
                                }
                                headers["Content-Type"] = content_type(boost::filesystem::extension(path));
                                headers["Content-Length"] = std::to_string(boost::filesystem::file_size(path));
                            }
                        }
                    }
                }
            }
        }
        catch (boost::system::error_code& ex) {
            code = 400;
        }
    }
    
    if (handle < 0) {
        std::make_shared<HTTPBufferResponseWriter>(
            std::move(server_), std::move(socket_),
            code, std::move(headers), ""
        )->start();
    }
    else {
        boost::asio::posix::stream_descriptor stream(socket_.get_io_context(), handle);
        std::make_shared<HTTPStreamResponseWriter>(
            std::move(server_), std::move(socket_),
            code, std::move(headers), std::move(stream)
        )->start();
    }

    stop();
}

