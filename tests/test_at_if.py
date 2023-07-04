import json
import netifaces
import os
import pathlib
import socket
import time
from conftest import apteryx_search, apteryx_get, apteryx_get_tree, apteryx_set

admin_status = {
    "admin-down": "0",
    "admin-up": "1",
}

oper_status = {
    "IF_OPER_UNKNOWN": "0",
    "IF_OPER_NOTPRESENT": "1",
    "IF_OPER_DOWN": "2",
    "IF_OPER_LOWERLAYERDOWN": "3",
    "IF_OPER_TESTING": "4",
    "IF_OPER_DORMANT": "5",
    "IF_OPER_UP": "6",
}


def test_at_if_get_iflist():
    ifaces = netifaces.interfaces()
    print(ifaces)
    output = apteryx_search('/interface/interfaces/')
    print(output)
    ifs = []
    for line in output.splitlines():
        ifs.append(pathlib.PurePath(line).name)
    print(ifs)
    assert sorted(ifaces) == sorted(ifs)


def test_at_if_default_fields():
    expected = {
        'name': 'eth0',
        'if-index': str(socket.if_nametoindex('eth0')),
        'l3': '1',
        'status': {
            'txq': '4',
            'duplex': '1',
            'txqlen': '1000',
            'rxq': '4',
            'phys-address': netifaces.ifaddresses('eth0')[netifaces.AF_LINK][0]['addr'],
            'speed': '10000',
            'qdisc': 'noqueue',
            'oper-status': oper_status["IF_OPER_UP"],
            'flags': '69699',
            'admin-status': admin_status["admin-up"],
            'mtu': '1500',
            'promisc': '0',
            'arptype': '1'
        }
    }
    tree = apteryx_get_tree('/interface/interfaces/eth0')
    print(json.dumps(tree, indent=4))
    assert tree['interface']['interfaces']['eth0'] == expected


def test_at_if_oper_status():
    assert apteryx_get('/interface/interfaces/eth0/status/oper-status') == oper_status["IF_OPER_UP"]
    os.system('sudo ip link set dev eth0 down')
    time.sleep(1)
    assert apteryx_get('/interface/interfaces/eth0/status/oper-status') == oper_status["IF_OPER_DOWN"]
    os.system('sudo ip link set dev eth0 up')
    time.sleep(1)
    assert apteryx_get('/interface/interfaces/eth0/status/oper-status') == oper_status["IF_OPER_UP"]


def test_at_if_admin_status():
    assert apteryx_get('/interface/interfaces/eth0/status/admin-status') == admin_status["admin-up"]
    assert apteryx_get('/interface/interfaces/eth0/status/oper-status') == oper_status["IF_OPER_UP"]
    apteryx_set('/interface/interfaces/eth0/settings/admin-status', admin_status["admin-down"])
    time.sleep(1)
    assert apteryx_get('/interface/interfaces/eth0/status/admin-status') == admin_status["admin-down"]
    assert apteryx_get('/interface/interfaces/eth0/status/oper-status') == oper_status["IF_OPER_DOWN"]
    apteryx_set('/interface/interfaces/eth0/settings/admin-status', admin_status["admin-up"])
    time.sleep(1)
    assert apteryx_get('/interface/interfaces/eth0/status/admin-status') == admin_status["admin-up"]
    assert apteryx_get('/interface/interfaces/eth0/status/oper-status') == oper_status["IF_OPER_UP"]
