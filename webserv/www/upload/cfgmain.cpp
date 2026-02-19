#include "Config.hpp"
#include <iostream>
int main() {
    Config c("config/webserv.conf");
    c.parse();
    const std::vector<ServerConfig>& s = c.getServers();
    std::cout << "Locations in server[0]:\n";
    for (size_t i = 0; i < s[0].locations.size(); ++i) {
        const LocationConfig& l = s[0].locations[i];
        std::cout << "  [" << i << "] path=" << l.path
                  << " root=" << l.root
                  << " upload_path=" << l.upload_path
                  << " autoindex=" << l.autoindex << "\n";
    }
}
