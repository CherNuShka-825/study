go mod . \[-iface interface\] \[-port port\] \<multicast-address\>
if run without -iface, you can see your network interfaces (if you have more then 1)

multicast for IPv4: 224.0.0.0/4
multicast for IPv6: ff00::/8
  ff01::/16   interface-local
  ff02::/16   link-local
  ff05::/16   site-local
  ff0e::/16   global
