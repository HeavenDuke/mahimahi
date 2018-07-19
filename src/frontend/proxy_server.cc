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

ProxyServer::ProxyServer(const Address &address):
    fbmap(),
    frontend(address),
    pid_file("/tmp/nghttpx.pid")
{
//    frontend = address;
//    fbmap = map<string, Address>();
}

ProxyServer::~ProxyServer()
{

}

void ProxyServer::Run()
{
    vector<string> commands;
    commands.push_back(NGHTTPX);
    commands.push_back("-f");
    commands.push_back(frontend.str(",") + (frontend.port() == 80 ? ";no-tls" : ""));
    for(map<string, Address>::iterator iter = fbmap.begin(); iter != fbmap.end(); iter++) {
        commands.push_back("-b");
        commands.push_back(iter->second.str(",") + ";" + iter->first + ";dns");
    }
    commands.push_back("-b");
    commands.push_back("127.0.0.1,3128");
    commands.push_back("--no-via");
    commands.push_back("--pid-file=" + pid_file);
    commands.push_back("-D");
    int count = commands.size();
    cout << "Command:";
    for (int i = 0; i < count;i++)
    {
        cout << " " << commands[i];
    }
    cout << endl;
    run(commands);
}

void ProxyServer::Stop()
{
    // TODO: Stop current nghttpx server here
    string pid;
    ifstream pidIn(pid_file);
    pidIn >> pid;
    vector<string> commands;
    commands.push_back(KILL);
    commands.push_back("-SIGTERM");
    commands.push_back(pid);
    cout << "Stop nghttpx:";
    for(auto w: commands) {
        cout << " " << w;
    }
    cout << endl;
    run(commands);
}

void ProxyServer::add_front_back_mapping(const string &host, const Address &backend)
{
    fbmap.insert(map<string, Address>::value_type (host, backend));
}

