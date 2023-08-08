import apteryx
import json
import os
import pytest
import subprocess
import time


WAIT_TIME = 0.25


hash_algorithms = {
    'layer2': 0,
    'layer2+3': 1,
    'layer3+4': 2,
    'encap2+3': 3,
    'encap3+4': 4,
}


@pytest.fixture(autouse=True)
def run_around_tests():
    print("Before test")
    yield
    print("After test")
    apteryx.prune("/bonding")
    apteryx.prune("/interface/interfaces/bond0")
    bond_del('bond0')
    bond_del('eth1')
    bond_del('eth2')


def bond_add(name, members=None, mode=None, policy=None):
    os.system("sudo ip link add dev " + name + " type bond")
    if mode is not None:
        os.system("sudo ip link set dev " + name + " type bond mode " + mode)
    if policy is not None:
        os.system("sudo ip link set dev " + name + " type bond xmit_hash_policy " + policy)
    if members is not None:
        for member in members:
            os.system("sudo ip link set dev " + member + " down")
            os.system("sudo ip link set dev " + member + " master " + name)


def bond_del(name):
    os.system("sudo ip link del dev " + name + " type bond")


def bond_status(name):
    status = subprocess.check_output(f"cat /proc/net/bonding/{name}", shell=True).strip().decode('utf-8')
    print(status)
    return status


def tap_add(name, up=False):
    os.system("sudo ip tuntap add mode tap name " + name)
    if up:
        os.system("sudo ip link set dev " + name + " up")


def tap_del(name):
    os.system("sudo ip tuntap del mode tap name " + name)


def tap_set(name, up=False):
    if up:
        os.system("sudo ip link set dev " + name + " up")
    else:
        os.system("sudo ip link set dev " + name + " down")


# Test bond test framework


def test_bonding_test_framework():
    tap_add('eth1')
    tap_add('eth2')
    bond_add('bond0', ['eth1', 'eth2'])
    status = bond_status('bond0')
    assert 'Bonding Mode: load balancing' in status
    assert 'Slave Interface: eth1' in status
    assert 'Slave Interface: eth2' in status
    bond_del('bond0')
    bond_add('bond0', ['eth1', 'eth2'], '802.3ad')
    status = bond_status('bond0')
    assert 'Bonding Mode: IEEE 802.3ad' in status
    assert 'Slave Interface: eth1' in status
    assert 'Slave Interface: eth2' in status
    bond_del('bond0')
    bond_add('bond0', None, '802.3ad', 'layer2')
    assert 'Hash Policy: layer2' in bond_status('bond0')
    bond_del('bond0')
    bond_add('bond0', None, '802.3ad', 'layer2+3')
    assert 'Hash Policy: layer2+3' in bond_status('bond0')
    bond_del('bond0')
    bond_add('bond0', None, '802.3ad', 'layer3+4')
    assert 'Hash Policy: layer3+4' in bond_status('bond0')
    bond_del('bond0')
    bond_add('bond0', None, '802.3ad', 'encap2+3')
    assert 'Hash Policy: encap2+3' in bond_status('bond0')
    bond_del('bond0')
    bond_add('bond0', None, '802.3ad', 'encap3+4')
    assert 'Hash Policy: encap3+4' in bond_status('bond0')
    bond_del('bond0')


# Create bonds


def test_bonding_create_bond_defaults():
    tree = {
        'bonding': {
            'bonds': {
                'bond0': {
                    'name': 'bond0',
                }
            }
        }
    }
    assert apteryx.set_tree(tree)
    time.sleep(WAIT_TIME)
    tree = apteryx.get_tree('/interface/interfaces/bond0')
    print(json.dumps(tree, indent=4))
    assert tree['bond0']['name'] == 'bond0'
    tree = apteryx.get_tree('/bonding/bonds/bond0')
    print(json.dumps(tree, indent=4))
    assert tree['bond0']['name'] == 'bond0'
    assert tree['bond0']['status']['hash-policy'] == '0'
    assert tree['bond0']['status']['mii-status'] == '0'
    # TODO
    # assert tree['bond0']['status']['system-mac'] == '0'
    # assert tree['bond0']['status']['actor-key'] == '0'
    # assert tree['bond0']['status']['partner-key'] == '0'
    # assert tree['bond0']['status']['partner-mac'] == '0'
    # assert tree['bond0']['status']['members'] == {}


def test_bonding_create_bond_lacp():
    tree = {
        'bonding': {
            'bonds': {
                'bond0': {
                    'name': 'bond0',
                    'settings': {
                        'type': 'lacp'
                    }
                }
            }
        }
    }
    assert apteryx.set_tree(tree)
    time.sleep(WAIT_TIME)
    assert 'Bonding Mode: IEEE 802.3ad' in bond_status('bond0')


def test_bonding_create_bond_members():
    tap_add('eth1')
    tap_add('eth2')
    tree = {
        'bonding': {
            'bonds': {
                'bond0': {
                    'name': 'bond0',
                    'settings': {
                        'members': [
                            'eth0',
                            'eth1'
                        ]
                    }
                }
            }
        }
    }
    assert apteryx.set_tree(tree)
    time.sleep(WAIT_TIME)
    tree = apteryx.get_tree('/interface/interfaces/bond0')
    print(json.dumps(tree, indent=4))
    assert tree['bond0']['name'] == 'bond0'
    assert tree['bond0']['status']['members'] == ['eth0', 'eth1']


def test_bonding_add_bond_members():
    tree = {
        'bonding': {
            'bonds': {
                'bond0': {
                    'name': 'bond0'
                }
            }
        }
    }
    assert apteryx.set_tree(tree)
    tree = {
        'bonding': {
            'bonds': {
                'bond0': {
                    'settings': {
                        'members': [
                            'eth1'
                        ]
                    }
                }
            }
        }
    }
    assert apteryx.set_tree(tree)
    time.sleep(WAIT_TIME)
    assert 'Slave Interface: eth1' in bond_status('bond0')


# Delete bonds


def test_bonding_delete_bond_simple():
    bond_add('bond0')
    time.sleep(WAIT_TIME)
    assert apteryx.prune('/bonding/bonds/bond0')
    time.sleep(WAIT_TIME)
    assert apteryx.get_tree('/interface/interfaces/bond0') == {}


def test_bonding_delete_bond_with_members():
    tap_add('eth1')
    tap_add('eth2')
    bond_add('bond0', ['eth1', 'eth2'])
    time.sleep(WAIT_TIME)
    assert apteryx.prune('/bonding/bonds/bond0')
    time.sleep(WAIT_TIME)
    assert apteryx.get_tree('/interface/interfaces/bond0') == {}
    assert apteryx.get_tree('/bonding/bonds/bond0') == {}


def test_bonding_delete_bond_members():
    tap_add('eth1')
    tap_add('eth2')
    bond_add('bond0', ['eth1', 'eth2'])
    time.sleep(WAIT_TIME)
    assert 'Slave Interface: eth1' in bond_status('bond0')
    assert 'Slave Interface: eth2' in bond_status('bond0')
    assert apteryx.prune('/bonding/bonds/bond0/settings/members/eth1')
    time.sleep(WAIT_TIME)
    assert 'Slave Interface: eth1' not in bond_status('bond0')
    assert 'Slave Interface: eth2' in bond_status('bond0')
    assert apteryx.prune('/bonding/bonds/bond0/settings/members/eth2')
    time.sleep(WAIT_TIME)
    assert 'Slave Interface: eth2' not in bond_status('bond0')


# Detect bonds


def test_bonding_detect_bond_simple():
    bond_add('bond0', mode="balance-rr")
    time.sleep(WAIT_TIME)
    tree = apteryx.get_tree('/interface/interfaces/bond0')
    print(json.dumps(tree, indent=4))
    assert tree['bond0']['name'] == 'bond0'
    tree = apteryx.get_tree('/bonding/bonds/bond0')
    print(json.dumps(tree, indent=4))
    assert tree['bond0']['name'] == 'bond0'


def test_bonding_detect_bond_static():
    bond_add('bond0', mode="balance-rr")
    time.sleep(WAIT_TIME)
    tree = apteryx.get_tree('/bonding/bonds/bond0')
    print(json.dumps(tree, indent=4))
    assert tree['bond0']['name'] == 'bond0'
    assert tree['bond0']['type'] is None


def test_bonding_detect_bond_lacp():
    bond_add('bond0', mode="802.3ad")
    time.sleep(WAIT_TIME)
    tree = apteryx.get_tree('/bonding/bonds/bond0')
    print(json.dumps(tree, indent=4))
    assert tree['bond0']['name'] == 'bond0'
    assert tree['bond0']['type'] == '0'


def test_bonding_detect_bond_members():
    tap_add('eth1')
    tap_add('eth2')
    bond_add('bond0', ['eth1', 'eth2'])
    tree = apteryx.get_tree('/interface/interfaces/bond0')
    print(json.dumps(tree, indent=4))
    assert tree['bond0']['name'] == 'bond0'
    tree = apteryx.get_tree('/bonding/bonds/bond0')
    print(json.dumps(tree, indent=4))
    assert tree['bond0']['name'] == 'bond0'
    assert tree['bond0']['status']['members'] == ['eth0', 'eth1']


def test_bonding_detect_bond_hash():
    for alg, val in hash_algorithms.items():
        bond_add('bond0', None, None, alg)
        time.sleep(WAIT_TIME)
        tree = apteryx.get_tree('/bonding/bonds/bond0')
        print(json.dumps(tree, indent=4))
        assert tree['bond0']['name'] == 'bond0'
        assert tree['bond0']['status']['hash-policy'] == str(val)


# Link status


def test_bonding_detect_link_down_up():
    tap_add('eth1', False)
    bond_add('bond0', ['eth1'])
    time.sleep(WAIT_TIME)
    tree = apteryx.get_tree('/bonding/bonds/bond0/status')
    print(json.dumps(tree, indent=4))
    assert tree['mii-status'] == '0'
    assert tree['members']['eth1']['mii-status'] == '0'
    tap_set('eth1', True)
    tree = apteryx.get_tree('/bonding/bonds/bond0/status')
    print(json.dumps(tree, indent=4))
    assert tree['mii-status'] == '1'
    assert tree['members']['eth1']['mii-status'] == '1'


def test_bonding_detect_link_up_down():
    tap_add('eth1', True)
    bond_add('bond0', ['eth1'])
    time.sleep(WAIT_TIME)
    tree = apteryx.get_tree('/bonding/bonds/bond0/status')
    print(json.dumps(tree, indent=4))
    assert tree['mii-status'] == '1'
    assert tree['members']['eth1']['mii-status'] == '1'
    tap_set('eth1', False)
    tree = apteryx.get_tree('/bonding/bonds/bond0/status')
    print(json.dumps(tree, indent=4))
    assert tree['mii-status'] == '0'
    assert tree['members']['eth1']['mii-status'] == '0'
