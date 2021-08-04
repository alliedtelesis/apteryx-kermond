"""

Output paths in C header file format

"""
import sys
import os.path
import re

from pyang import plugin
from pyang import statements
from pyang import error


def pyang_plugin_init():
    plugin.register_plugin(PathPlugin())


class PathPlugin(plugin.PyangPlugin):

    def add_output_format(self, fmts):
        self.multiple_modules = True
        fmts['cpaths'] = self

    def emit(self, ctx, modules, fd):
        for module in modules:
            children = [child for child in module.i_children]
            for child in children:
                print_node(child, module, fd, ctx)
        fd.write('\n')


def mk_path_str(s, level=0, strip=0, fd=None):
    #print('LEVEL: ' + str(level) + ' STRIP:' + str(strip) + ' ' + s.arg)
    if level > strip:
        p = mk_path_str(s.parent, level - 1, strip)
        if p != "":
            return p + "/" + s.arg
        return s.arg
    if level == strip:
        return s.arg
    else:
        return ""


def mk_path_str_define(s):
    if s.parent.keyword in ['module', 'submodule']:
        return "/" + s.arg
    else:
        p = mk_path_str_define(s.parent)
        return p + "/" + s.arg


def print_node(node, module, fd, ctx, level=0, strip=0):
    #print('TYPE:' + node.keyword + 'LEVEL:' + str(level) + 'STRIP:' + str(strip))

    # No need to include these nodes
    if node.keyword in ['rpc', 'notification']:
        return

    # Strip all nodes from the path at list items
    if node.parent.keyword == 'list':
        strip = level

    # Create path value
    value = mk_path_str(node, level, strip, fd)
    if strip == 0:
        value = "/" + value

    # Create define
    if node.keyword in ['choice', 'case']:
        pathstr = mk_path_str_define(node)
    else:
        pathstr = statements.mk_path_str(node, False)

    if node.keyword == 'container' or node.keyword == 'list':
        define = pathstr[1:].upper().replace('/', '_').replace('-', '_') + '_PATH'
    else:
        define = pathstr[1:].upper().replace('/', '_').replace('-', '_')

    # Description
    descr = node.search_one('description')
    if descr is not None:
        descr.arg = descr.arg.replace('\r', ' ').replace('\n', ' ')
        fd.write('/* ' + descr.arg + ' */\n')

    # Ouput define
    fd.write('#define ' + define + ' "' + value + '"\n')

    type = node.search_one('type')
    if type is not None:
        if type.arg == 'boolean':
            fd.write('#define ' + define + '_TRUE "true"\n')
            fd.write('#define ' + define + '_FALSE "false"\n')

        if type.arg == 'enumeration':
            count = 0
            for enum in type.substmts:
                val = enum.search_one('value')
                if val is not None:
                    fd.write('#define ' + define + '_' + enum.arg.upper().replace('-', '_') + ' ' + str(val.arg) + '\n')
                    try:
                        val_int = int(val.arg)
                    except:
                        val_int = None
                    if val_int is not None:
                        count = val_int
                else:
                    fd.write('#define ' + define + '_' + enum.arg.upper().replace('-', '_') + ' ' + str(count) + '\n')
                count = count + 1

    # Default value
    def_val = node.search_one('default')
    if def_val is not None:
        if type is not None and 'int' in type.arg:
            fd.write('#define ' + define + '_DEFAULT ' + def_val.arg + '\n')
        elif type is not None and type.arg == 'enumeration':
            for enum in type.substmts:
                val = enum.search_one('value')
                if def_val.arg == enum.arg and val is not None:
                    fd.write('#define ' + define + '_DEFAULT ' + val.arg + '\n')
        else:
            fd.write('#define ' + define + '_DEFAULT "' + def_val.arg + '"\n')

    # Process children
    if hasattr(node, 'i_children'):
        for child in node.i_children:
            print_node(child, module, fd, ctx, level + 1, strip)
