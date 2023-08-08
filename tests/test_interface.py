import json
import netifaces
import os
import socket
import time
import apteryx

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


def test_interface_get_iflist():
    ifaces = netifaces.interfaces()
    print(ifaces)
    output = apteryx.search('/interface/interfaces/')
    print(output)
    output = [str(i).replace('/interface/interfaces/', '') for i in output]
    assert sorted(ifaces) == sorted(output)


def test_interface_default_fields():
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
    tree = apteryx.get_tree('/interface/interfaces/eth0')
    print(json.dumps(tree, indent=4))
    assert tree['interface']['interfaces']['eth0'] == expected


def test_interface_oper_status():
    assert apteryx.get('/interface/interfaces/eth0/status/oper-status') == oper_status["IF_OPER_UP"]
    os.system('sudo ip link set dev eth0 down')
    time.sleep(1)
    assert apteryx.get('/interface/interfaces/eth0/status/oper-status') == oper_status["IF_OPER_DOWN"]
    os.system('sudo ip link set dev eth0 up')
    time.sleep(1)
    assert apteryx.get('/interface/interfaces/eth0/status/oper-status') == oper_status["IF_OPER_UP"]


def test_ainterface_admin_status():
    assert apteryx.get('/interface/interfaces/eth0/status/admin-status') == admin_status["admin-up"]
    assert apteryx.get('/interface/interfaces/eth0/status/oper-status') == oper_status["IF_OPER_UP"]
    apteryx.set('/interface/interfaces/eth0/settings/admin-status', admin_status["admin-down"])
    time.sleep(1)
    assert apteryx.get('/interface/interfaces/eth0/status/admin-status') == admin_status["admin-down"]
    assert apteryx.get('/interface/interfaces/eth0/status/oper-status') == oper_status["IF_OPER_DOWN"]
    apteryx.set('/interface/interfaces/eth0/settings/admin-status', admin_status["admin-up"])
    time.sleep(1)
    assert apteryx.get('/interface/interfaces/eth0/status/admin-status') == admin_status["admin-up"]
    assert apteryx.get('/interface/interfaces/eth0/status/oper-status') == oper_status["IF_OPER_UP"]
