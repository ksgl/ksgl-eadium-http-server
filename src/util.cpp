#include "util.hpp"

#include <boost/algorithm/string.hpp>

#include <ctime>


const std::regex request_line_rx_g{R"(^(\S+)\s+(.*?)\s+(HTTP/\d+(?:\.\d+)?)$)", std::regex_constants::ECMAScript | std::regex_constants::optimize};
const std::regex header_rx_g{R"(^(\S+):\s+(.*?)\s*$)", std::regex_constants::ECMAScript | std::regex_constants::optimize};

std::string make_printable(const std::string& str) {
    constexpr std::size_t max_len = 1000;
    std::string result;
    
    if (str.size() > max_len) {
        result = str.substr(0, max_len);
        result += "<truncated>";
    }
    else {
        result = str;
    }
    
    
    
    // TODO: escape non-printable chars
    
    
    
    
    return result;
}

std::string code_text(int code) {
    switch (code) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        default: return "Unknown";
    }
}

std::string rfc_1123_now() {
    ::time_t t;
    ::time(&t);
    auto tm = ::gmtime(&t);
    
    char buf[512] = { 0 };
    auto res = ::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", tm);
    if (res <= 0) {
        throw std::runtime_error("strptime() failed");
    }

    return std::string(buf, res);
}

std::string content_type(std::string ext) {
    boost::algorithm::to_lower(ext);
    
    if (ext == ".htm" || ext == ".html") return "text/html";
    else if (ext == ".css") return "text/css";
    else if (ext == ".js") return "application/javascript";
    else if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    else if (ext == ".png") return "image/png";
    else if (ext == ".gif") return "image/gif";
    else if (ext == ".swf") return "application/x-shockwave-flash";
    else if (ext == ".txt") return "text/plain";
    
    return "application/octet-stream";
}

std::string remove_request_part(const std::string& url) {
    auto start = url.find('?');
    if (start == std::string::npos) {
        return url;
    }
    else {
        return url.substr(0, start);
    }
}

bool url_decode(const std::string& in, std::string& out) {
    out.clear();
    out.reserve(in.size());
    for (std::size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%') {
            if (i + 3 <= in.size()) {
                int value = 0;
                std::istringstream is(in.substr(i + 1, 2));
                if (is >> std::hex >> value) {
                    out += static_cast<char>(value);
                    i += 2;
                }
                else {
                    return false;
                }
            }
            else {
                return false;
            }
        }
        else if (in[i] == '+') {
            out += ' ';
        }
        else {
            out += in[i];
        }
    }
    return true;
}

// Assuming both paths are canonicalized.
bool is_in(const boost::filesystem::path& path, const boost::filesystem::path& base) {
    auto path_len = std::distance(path.begin(), path.end());
    auto base_len = std::distance(base.begin(), base.end());

    if (base_len > path_len) {
        return false;
    }

    return std::equal(base.begin(), base.end(), path.begin());
}

