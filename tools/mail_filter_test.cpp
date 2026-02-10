#include "bonus/Mail.hpp"
#include <iostream>

int main() {
    SmtpConfig cfg;
    cfg.enabled = true;
    cfg.minLevel = LEVEL_ERROR;
    cfg.keywords.push_back("panic");
    cfg.host = "localhost";
    cfg.to.push_back("root@localhost");

    if (!should_send_mail(cfg, LEVEL_ERROR, "panic: disk full")) {
        std::cout << "FAIL: expected send for panic" << std::endl;
        return 1;
    }
    if (should_send_mail(cfg, LEVEL_LOG, "panic: disk full")) {
        std::cout << "FAIL: expected no send for LOG" << std::endl;
        return 1;
    }
    if (should_send_mail(cfg, LEVEL_ERROR, "all good")) {
        std::cout << "FAIL: expected no send without keyword" << std::endl;
        return 1;
    }

    std::cout << "OK" << std::endl;
    return 0;
}
