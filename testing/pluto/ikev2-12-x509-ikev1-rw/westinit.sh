/testing/guestbin/swan-prep --x509
ipsec _stackmanager start
# confirm that the network is alive
ping -n -c 4 -I 192.0.1.254 192.0.2.254
# make sure that clear text does not get through
iptables -A INPUT -i eth1 -s 192.0.2.0/24 -j LOGDROP
export EF_DISABLE_BANNER=1
/usr/local/libexec/ipsec/pluto --config /etc/ipsec.conf
/testing/pluto/bin/wait-until-pluto-started
ipsec auto --add westnet-eastnet-ikev2
ipsec auto --status
echo "initdone"
