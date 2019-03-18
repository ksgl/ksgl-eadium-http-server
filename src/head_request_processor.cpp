#include "head_request_processor.hpp"
#include "http_response_writer.hpp"
#include "util.hpp"

#include <boost/filesystem.hpp>


HEADRequestProcessor::~HEADRequestProcessor() {
    stop();
}

void HEADRequestProcessor::start() {
    if (!self) {
        self = shared_from_this();
        send_response();
    }
}

void HEADRequestProcessor::stop() noexcept {
    if (self) {
        boost::system::error_code ec;
        socket_.cancel(ec);
        socket_.close(ec);
        HEADRequestProcessorPtr haron;
        haron.swap(self);
    }
}

void HEADRequestProcessor::send_response() {
    int code = 400;
    std::map<std::string, std::string> headers;

    headers["Server"] = server_->get_server_string();
    headers["Connection"] = "close";
    headers["Date"] = rfc_1123_now();

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
                        code = 404;
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
        catch (boost::system::error_code& ex) {
            code = 400;
        }
    }

    std::make_shared<HTTPBufferResponseWriter>(
        std::move(server_), std::move(socket_),
        code, std::move(headers), ""
    )->start();

    stop();
}

