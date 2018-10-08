/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <fstream>

#include "proxy_server.hh"
#include "apache_configuration.hh"
#include "system_runner.hh"
#include "config.h"
#include "util.hh"
#include "exception.hh"

using namespace std;

ProxyServer::ProxyServer(const Address &_frontend, const Address &_backend) :
        frontend(_frontend),
        backend(_backend),
        config_file_("/tmp/replayshell_nghttpx_config"),
        pid_file_("/tmp/replayshell_nghttpx_pid_" + to_string(getpid()) + "." + to_string(random())),
        moved_away_(false) {
    config_file_.write("frontend=" + frontend.str(",") + (frontend.port() == 443 ? "" : ";no-tls") + "\n");

    config_file_.write("backend=" + backend.str(",") + (frontend.port() == 443 ? ";;tls" : "") + "\n");

    config_file_.write("pid-file=" + pid_file_ + "\n");

    config_file_.write("accesslog-file=/var/log/nghttpx/access.log\n");

    config_file_.write("errorlog-file=/var/log/nghttpx/error.log\n");

    // Enable TLS configuration
    if (frontend.port() == 443) {
        config_file_.write(proxy_ssl_config);
    }

    run({NGHTTPX, "-D", "-k", "--conf", config_file_.name()});
}

ProxyServer::~ProxyServer() {
    if (moved_away_) { return; }
    string pid;
    ifstream pidIn(pid_file_);
    pidIn >> pid;
    try {
        run({KILL, "-SIGQUIT", pid});
    } catch (const exception &e) { /* don't throw from destructor */
        print_exception(e);
    }
}

ProxyServer::ProxyServer(ProxyServer &&other)
        : frontend(other.frontend),
          backend(other.backend),
          config_file_(move(other.config_file_)),
          pid_file_(other.pid_file_),
          moved_away_(false) {
    other.moved_away_ = true;
}
