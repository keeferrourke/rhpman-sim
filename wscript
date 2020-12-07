def build(bld):
    obj = bld.create_ns3_program('', ['stats', 'dsdv', 'internet', 'mobility', 'wifi'])
    obj.source = ['logging.cc', 'main.cc', 'nsutil.cc', 'rhpman.cc', 'simulation-area.cc', 'simulation-params.cc']
