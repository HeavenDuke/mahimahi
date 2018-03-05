/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef ADDRESS_HH
#define ADDRESS_HH

#include <string>
#include <utility>

#include <netinet/in.h>
#include <netdb.h>

/* Address class for IPv4/v6 addresses */
class Address
{
public:
    typedef union {
        sockaddr as_sockaddr;
        sockaddr_storage as_sockaddr_storage;
    } raw;

private:
    socklen_t size_;
    raw addr_;

    /* private constructor given ip/host, service/port, and optional hints */
    Address( const std::string & node, const std::string & service, const addrinfo & hints );

public:
    /* constructors �Ը�����ֵ����ƹ��� */
    Address();
    Address( const raw & addr, const size_t size );
    Address( const sockaddr & addr, const size_t size );
    Address( const sockaddr_in & addr );

    /* construct by resolving host name and service name */
    Address( const std::string & hostname, const std::string & service ); // �������ͷ����������ַ����Ż��Դ�������

    /* construct with numerical IP address and numeral port number */
    Address( const std::string & ip, const uint16_t port ); // ��IP��ַ�Ͷ˿ںŹ���һ����ַ

    /* accessors */
    std::pair<std::string, uint16_t> ip_port( void ) const;  // ����IP�Ͷ˿ں�
    std::string ip( void ) const { return ip_port().first; } // ����IP��ַ
    uint16_t port( void ) const { return ip_port().second; } // ���ض˿ں�
    std::string str( const std::string port_separator = ":" ) const; // �����ַ���ַ�����ʽ

    socklen_t size( void ) const { return size_; }
    const sockaddr & to_sockaddr( void ) const;

    /* comparisons */
    bool operator==( const Address & other ) const;
    bool operator<( const Address & other ) const;

    /* generate carrier-grade NAT address */
    static Address cgnat( const uint8_t last_octet );
};

#endif /* ADDRESS_HH */
