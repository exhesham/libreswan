/testing/guestbin/swan-prep
ipsec start
/testing/pluto/bin/wait-until-pluto-started
ipsec whack --debug-all --impair-send-bogus-isakmp-flag
ipsec auto --add westnet-eastnet
ipsec auto --status | grep westnet-eastnet
echo "initdone"
