def build(bld):
    obj = bld.create_ns3_program('rhpman', ['stats', 'dsdv', 'internet', 'mobility', 'wifi'])
    obj.source = ['rhpman.cc', 'grid-divider.cc']
