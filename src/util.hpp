#pragma once

#include <boost/filesystem.hpp>

#include <regex>
#include <string>


#define CRLF "\r\n"

extern const std::regex request_line_rx_g;
extern const std::regex header_rx_g;

std::string make_printable(const std::string& str);
std::string code_text(int code);
std::string rfc_1123_now();
std::string content_type(std::string ext);
std::string remove_request_part(const std::string& url);
bool url_decode(const std::string& in, std::string& out);
bool is_in(const boost::filesystem::path& path, const boost::filesystem::path& base);

