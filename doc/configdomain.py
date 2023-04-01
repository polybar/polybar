# -*- coding: utf-8 -*-
"""
    configdomain
    ~~~~~~~~~~~~

    TODO document
"""

from sphinxcontrib.domaintools import custom_domain
from sphinx.util.docfields import Field, GroupedField

__version__ = "0.1.0"
# for this module's sphinx doc
release = __version__
version = release.rsplit('.', 1)[0]


myDomain = custom_domain('PolybarConfigDomain',
                         name='polybar',
                         label="Polybar Configuration",

                         elements=dict(
                             setting=dict(
                                 objname="Config Setting",
                                 fields=[
                                     Field('type',
                                           label="Type",
                                           names=['type'],
                                           has_arg=False,
                                           ),
                                     Field('tags',
                                           label="Available Tags",
                                           names=['tags'],
                                           has_arg=False,
                                           ),
                                     Field('tokens',
                                           label="Supported Tokens",
                                           names=['tokens'],
                                           has_arg=False,
                                           ),
                                     Field('default',
                                           label="Default Value",
                                           names=['default'],
                                           has_arg=False,
                                           ),
                                 ]
                             ),
                         ))
