#include <iostream>
#include <fstream>
#include <string.h>


typedef struct Settings {
    int threads_count;
    int cpu_limit;
    int port;
    std::string document_root;
} Settings;

class CfgParser {
private:
    int threads_count;
    int cpu_limit;
    int port;
    std::string root_dir;
    std::string conf_path;
public:
    CfgParser(std::string path);
    void parse_config();
    Settings* get_setting();
};

CfgParser::CfgParser(std::string path) {
    path = "/etc/httpd.conf";
    this->conf_path = path;
    this->threads_count = 256;
    this->cpu_limit = 4;
    this->port = 80;
    this->root_dir.assign("/var/www/html");
}

void CfgParser::parse_config() {
    std::ifstream cfg(this->conf_path.c_str(), std::ios::in);

    if (cfg.is_open()) {
        char* c = new char[100];
        std::streamsize n = 100;

        for (size_t i = 0; i < 4 && cfg.good(); i++) {
            cfg.getline(c, n);
            std::string data(c);

            std::size_t found = data.find_last_of(' ');

            if (strstr(data.substr(0, found).c_str(), "document_root")) {
                this->root_dir.assign(data.substr(found + 1));
            }

            if (strstr(data.substr(0, found).c_str(), "thread_limit")) {
                this->threads_count = std::atoi(data.substr(found + 1).c_str());
            }

            if (strstr(data.substr(0, found).c_str(), "port")) {
                this->port = std::atoi(data.substr(found + 1).c_str());
            }

            if (strstr(data.substr(0, found).c_str(), "cpu_limit")) {
                this->cpu_limit = std::atoi(data.substr(found + 1).c_str());
            }

        }

        cfg.close();
        delete[] (c);
    }
}

Settings* CfgParser::get_setting() {
    Settings* cfg = new Settings;
    cfg->threads_count = this->threads_count;
    cfg->cpu_limit = this->cpu_limit;
    cfg->document_root = this->root_dir;
    cfg->port = this->port;
    return cfg;
}

