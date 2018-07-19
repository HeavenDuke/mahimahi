/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef PROXY_SERVER_HH
#define PROXY_SERVER_HH

#include <string>
#include <map>
#include "address.hh"

using namespace std;

class ProxyServer
{
private:
    map<string, Address> fbmap;
    Address frontend;
    const string pid_file;

public:
    ProxyServer(const Address & addr);
    ~ProxyServer();

    void Run();
    void Stop();
    void add_front_back_mapping(const string &host, const Address &backend);

};

#endif
