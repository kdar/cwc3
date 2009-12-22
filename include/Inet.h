//This was ripped from /usr/include/netinet/*

#ifndef _INET_H
#define _INET_H

#define _BSD_SOURCE 1
#define __FAVOR_BSD 1
#define __USE_BSD 1

#ifdef PLATFORM_UNIX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/tcp.h>
#endif

//-------------------------------
//Ethernet header
struct net_ethernet
{
  u_char ether_dhost[ETHER_ADDR_LEN]; //Destination host address
  u_char ether_shost[ETHER_ADDR_LEN]; //Source host address
  u_short ether_type; //IP? ARP? RARP? etc
};

//-------------------------------
//IP header
/*struct net_ip
{
  #if BYTE_ORDER == LITTLE_ENDIAN
  u_int ip_hl:4, //header length
  ip_v:4; //version
  #if BYTE_ORDER == BIG_ENDIAN
  u_int ip_v:4, //version
  ip_hl:4; //header length
  #endif
  #endif //not _IP_VHL
  u_char ip_tos; //type of service
  u_short ip_len; //total length
  u_short ip_id; //identification
  u_short ip_off; //fragment offset field
  #define IP_RF 0x8000 //reserved fragment flag
  #define IP_DF 0x4000 //dont fragment flag
  #define IP_MF 0x2000 //more fragments flag
  #define IP_OFFMASK 0x1fff //mask for fragmenting bits
  u_char ip_ttl; //time to live
  u_char ip_p; //protocol
  u_short ip_sum; //checksum
  struct in_addr ip_src, ip_dst; //source and dest address
};*/

//-------------------------------
//IP header
struct net_ip
{
  u_int8_t ip_vhl;  /* header length, version */
#define IP_V(ip) (((ip)->ip_vhl & 0xf0) >> 4)
#define IP_HL(ip) ((ip)->ip_vhl & 0x0f)
//IP_HL(ip) gives the header length as a count of 4-byte words.
//So if you want the length in bytes then multiply this value by 4.
#define IP_BYTE_HL(ip) (IP_HL(ip)*4)
  u_int8_t ip_tos;  /* type of service */
  u_int16_t ip_len;  /* total length */
  u_int16_t ip_id;  /* identification */
  u_int16_t ip_off;  /* fragment offset field */
#define IP_DF 0x4000   /* dont fragment flag */
#define IP_MF 0x2000   /* more fragments flag */
#define IP_OFFMASK 0x1fff  /* mask for fragmenting bits */
  u_int8_t ip_ttl;  /* time to live */
  u_int8_t ip_p;  /* protocol */
  u_int16_t ip_sum;  /* checksum */
  struct in_addr ip_src, ip_dst; /* source and dest address */
};

//-------------------------------
//TCP header
struct net_tcp
{
  u_short th_sport; //source port
  u_short th_dport; //destination port
  u_int32_t th_seq; //sequence number
  u_int32_t th_ack; //acknowledgement number
  /*#if BYTE_ORDER == LITTLE_ENDIAN
  u_int th_x2:4, //(unused)
  th_off:4; //data offset
  #endif
  #if BYTE_ORDER == BIG_ENDIAN
  u_int th_off:4, //data offset
  th_x2:4; //(unused)
  #endif*/
  u_char th_offx2; /* data offset, rsvd */
#define TH_OFF(th) (((th)->th_offx2 & 0xf0) >> 4)
  //TH_OFF(th) gives the header length as a count of 4-byte words.
  //So if you want the length in bytes then multiply this value by 4.
#define TH_BYTE_OFF(th) (TH_OFF(th)*4)
  u_char th_flags;
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80
#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
  u_short th_win; //window
  u_short th_sum; //checksum
  u_short th_urp; //urgent pointer
};

//-------------------------------
//SLL header
#define SLL_ADDRLEN 8         /* length of address field */
struct net_sll
{
  u_int16_t sll_pkttype;      /* packet type */
#define LINUX_SLL_HOST    0
#define LINUX_SLL_BROADCAST 1
#define LINUX_SLL_MULTICAST 2
#define LINUX_SLL_OTHERHOST 3
#define LINUX_SLL_OUTGOING 4
  u_int16_t sll_hatype;       /* link-layer address type */
  u_int16_t sll_halen;        /* link-layer address length */
  u_int8_t sll_addr[SLL_ADDRLEN]; /* link-layer address */
  u_int16_t sll_protocol;     /* protocol */
};

#endif
