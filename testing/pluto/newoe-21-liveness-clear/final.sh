ipsec whack --trafficstatus
ipsec whack --shuntstatus
ipsec look
ipsec look
grep "message ID:" /tmp/pluto.log
# grep on east
hostname |grep west > /dev/null || grep -A 1 "liveness_check - peer has not responded in" OUTPUT/east.pluto.log
# A tunnel should have established
grep "negotiated connection" /tmp/pluto.log
: ==== cut ====
ipsec auto --status
: ==== tuc ====
../bin/check-for-core.sh
if [ -f /sbin/ausearch ]; then ausearch -r -m avc -ts recent ; fi
: ==== end ====
