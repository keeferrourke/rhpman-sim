# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-



# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('rhpman', ['core', 'stats', 'aodv', 'dsdv', 'internet', 'mobility', 'wifi'])
    module.source = [
        'model/rhpman.cc',
        'model/dataItem.cc',
        'model/storage.cc',
        'model/nsutil.cc',
        'model/logging.cc',
        'helper/rhpman-helper.cc',
       'model/proto/messages.proto',
        ]

    module.cxxflags =  ['-I./contrib/rhpman/model']

    module_test = bld.create_ns3_module_test_library('rhpman')
    module_test.source = [
        'test/rhpman-test-suite.cc',
        ]
    # Tests encapsulating example programs should be listed here
    if (bld.env['ENABLE_EXAMPLES']):
        module_test.source.extend([
        #    'test/rhpman-examples-test-suite.cc',
             ])

    headers = bld(features='ns3header')
    headers.module = 'rhpman'
    headers.source = [
        'model/rhpman.h',
        'model/storage.h',
        'model/dataItem.h',
      #  'model/util.h',
        'helper/rhpman-helper.h',
       # 'model/proto/messages.proto',
      #  'model/messages.pb.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()







##
# This code is taken from:
# https://stackoverflow.com/questions/11274070/integrate-protocol-buffers-into-waf
###

from waflib.Task import Task
from waflib.TaskGen import extension

"""
A simple tool to integrate protocol buffers into your build system.

    def configure(conf):
        conf.load('compiler_cxx cxx protoc_cxx')

    def build(bld):
    bld.program(source = "main.cpp file1.proto proto/file2.proto",
            target = "executable")

"""

class protoc(Task):
    run_str = '${PROTOC} ${SRC} --cpp_out=. -I..'
    color = 'BLUE'
    ext_out = ['pb.h', 'pb.cc']

@extension('.proto')
def process_protoc(self, node):
    cpp_node = node.change_ext('.pb.cc')
    hpp_node = node.change_ext('.pb.h')
    self.create_task('protoc', node, [cpp_node, hpp_node])
    self.source.append(cpp_node)
    self.env.append_value('INCLUDES', ['.'] )

    self.use = self.to_list(getattr(self, 'use', '')) + ['PROTOBUF']

def configure(conf):
    conf.check_cfg(package="protobuf", uselib_store="PROTOBUF",
            args=['protobuf >= 3.0.0' '--cflags', '--libs'])
    conf.find_program('protoc', var='PROTOC')
