#!/bin/bash
NETNS=ssvpn
TUN=tun0
SOCKS=127.0.0.1:1080

mkdir -p /etc/netns/$NETNS/
echo 'nameserver 127.0.0.1' > /etc/netns/$NETNS/resolv.conf

tun2socks \
	--netif-ipaddr 192.168.111.1 \
	--netif-netmask 255.255.255.0 \
	--socks-server-addr $SOCKS \
	--tundev $TUN > /tmp/tun2socks.log &

sleep 1

ip netns add $NETNS
ip link set dev $TUN netns $NETNS

ip netns exec $NETNS ip link set lo up
ip netns exec $NETNS ip link set $TUN up
ip netns exec $NETNS ip addr add 192.168.111.2/24 dev $TUN
ip netns exec $NETNS ip ro add default dev $TUN

ip netns exec $NETNS dnsp &
