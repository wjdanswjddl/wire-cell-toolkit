#! /usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2011 (ita)
# stolen from local_rpath to be more global
#
# The warning against using this in production code goes double.

import copy
from waflib.TaskGen import after_method, feature

@after_method('propagate_uselib_vars')
#@feature('cprogram', 'cshlib', 'cxxprogram', 'cxxshlib', 'fcprogram', 'fcshlib')
@feature('cxxprogram', 'cxxshlib')
def add_rpath_stuff(self):
    all = copy.copy(self.to_list(getattr(self, 'use', [])))
    while all:
        name = all.pop()
        try:
            tg = self.bld.get_tgen_by_name(name)
        except:
            self.env['RPATH'] += self.env['LIBPATH_'+name.upper()]
            continue
        if hasattr(tg, 'link_task'):
            self.env.append_value('RPATH', tg.link_task.outputs[0].parent.abspath())
            all.extend(self.to_list(getattr(tg, 'use', [])))
