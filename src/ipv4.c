#include "arp.h"
#include "ipv4.h"
#include "icmpv4.h"
#include "netdev.h"

void ipv4_incoming(struct netdev *netdev, struct eth_hdr *hdr)
{
    struct iphdr *iphdr = (struct iphdr *) hdr->payload;

    if (iphdr->version != IPV4) {
        perror("Datagram version was not IPv4\n");
        return;
    }

    /* Convert fields from network byte order (Big Endian) to host network order */
    iphdr->len = ntohs(iphdr->len);
    iphdr->id = ntohs(iphdr->id);
    iphdr->flags = ntohs(iphdr->flags);
    iphdr->csum = ntohs(iphdr->csum);

    if (iphdr->ihl < 5) {
        perror("IPv4 header length must be at least 5\n");
        return; 
    }

    if (iphdr->ttl == 0) {
        //TODO: Send ICMP error
        perror("Time to live of datagram reached 0\n");
        return;
    }

    // TODO: Check fragmentation, possibly reassemble

    switch (iphdr->proto) {
    case ICMPV4:
        icmpv4_incoming(netdev, hdr);
        break;
    default:
        perror("Unknown IP header proto\n");
        return;
    }
}

void ipv4_outgoing(struct netdev *netdev, struct eth_hdr *hdr)
{
    struct iphdr *iphdr = (struct iphdr *)hdr->payload;
    uint32_t tmpaddr;
    uint8_t len = iphdr->len;

    /* Just swap the source and destination IP addresses,
     * don't bother with ARP lookup just now
     */
    tmpaddr = iphdr->saddr;
    iphdr->daddr = tmpaddr;
    iphdr->saddr = netdev->addr;

    /*
     * Switch back the necessary fields to Network Byte Order
     */
    iphdr->len = htons(iphdr->len);
    iphdr->id = ntohs(iphdr->id);
    iphdr->flags = ntohs(iphdr->flags);
    iphdr->csum = ntohs(iphdr->csum);

    netdev_transmit(netdev, hdr, ETH_P_IP, len, hdr->smac);
}