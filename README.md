
# KERMOND - Kernel Monitor Daemon
An Apteryx abstraction for the Linux kernel networking stack

## Requires
```
apteryx apteryx-xml glib-2.0 libnl-3.0 libnl-route-3.0
```

## Building
```
make
make install
```

## Running
```
$ ./apteryx-kermond -h
Usage: ./apteryx-kermond [-h] [-b] [-v] [-d] [-p <pidfile>]
  -h   show this help
  -b   background mode
  -d   enable debug
  -v   enable verbose debug
  -m   comma separated list of modules to load (e.g. ifconfig,ifstatus)
  -p   use <pidfile> (defaults to /var/run/apteryx-kermond.pid)
Modules: ifstatus ifconfig rib fib neighbor-settings static-neighbor neighbor-cache icmp tcp dot1q 
```

## Example - manage interfaces (interface.xml)
```
# Start daemon to manage interface configuration
apteryxd -b
apteryx-kermond -b -mifconfig,ifstatus

# Set admin-down
apteryx -s /interface/interfaces/eth1/settings/admin-status 0
# Set admin-up
apteryx -s /interface/interfaces/eth1/settings/admin-status 1
# Check oper-state
apteryx -g /interface/interfaces/eth1/status/oper-status
6
```

## Unit tests (using g_test)
```
make test
google-chrome gcov/index.html
```
