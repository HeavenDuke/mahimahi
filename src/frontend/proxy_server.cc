/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <iostream>
#include <unistd.h>

#include "proxy_server.hh"
#include "apache_configuration.hh"
#include "system_runner.hh"
#include "config.h"
#include "util.hh"
#include "exception.hh"

using namespace std;

ProxyServer::ProxyServer(const Address &address): fbmap(), frontend(address)
{
//    frontend = address;
//    fbmap = map<string, Address>();
}

ProxyServer::~ProxyServer()
{
//    try {
//        run( { APACHE2, "-f", config_file_.name(), "-k", "graceful-stop" } );
//    } catch ( const exception & e ) { /* don't throw from destructor */
//        print_exception( e );
//    }
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

void ProxyServer::add_front_back_mapping(const string &host, const Address &backend)
{
    fbmap.insert(map<string, Address>::value_type (host, backend));
}