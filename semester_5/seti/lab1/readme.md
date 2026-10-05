224.0.0.0 — 239.255.255.255
ff01::/16  interface-local
ff02::/16  link-local
ff05::/16  site-local
ff08::/16  organization-local
ff0e::/16  global

go doc net | grep -i multicast
grep -R "MulticastTCP" "$(go env GOROOT)/src
