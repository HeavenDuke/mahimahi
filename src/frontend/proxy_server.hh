/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef PROXY_SERVER_HH
#define PROXY_SERVER_HH

#include <string>

#include "temp_file.hh"
#include "address.hh"

using namespace std;

class ProxyServer
{
private:
    Address frontend, backend;
    TempFile config_file_;
    string pid_file_;
    bool moved_away_;
public:
    ProxyServer(const Address &frontend, const Address &backend);
    ~ProxyServer();

    /* ban copying */
    ProxyServer( const ProxyServer & other ) = delete;
    ProxyServer & operator=( const ProxyServer & other ) = delete;

    /* allow move constructor */
    ProxyServer( ProxyServer && other );

    /* ... but not move assignment operator */
    ProxyServer & operator=( ProxyServer && other ) = delete;
};

#endif
