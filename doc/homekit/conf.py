# Kconfig documentation build configuration file

import sys
import os
import os.path as path

NRF_BASE = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

if "ZEPHYR_BASE" not in os.environ:
    sys.exit("$ZEPHYR_BASE environment variable undefined.")
ZEPHYR_BASE = os.path.abspath(os.environ["ZEPHYR_BASE"])

# Add the Zephyr 'extensions' directory to sys.path, to enable finding
# Sphinx extensions within.
sys.path.insert(0, os.path.join(ZEPHYR_BASE, 'doc', 'extensions'))

# Let Sphinx find our extensions.
sys.path.append(os.path.join(NRF_BASE, 'scripts', 'sphinx_extensions'))

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = ['sphinx.ext.intersphinx',
              'breathe',
              'interbreathe',
              'table_from_rows',
              'options_from_kconfig',
              'sphinx.ext.ifconfig',
              'sphinxcontrib.mscgen',
              'sphinx_tabs.tabs',
              'zephyr.html_redirects']



# if "NRF_BASE" not in os.environ:
#     sys.exit("$NRF_BASE environment variable undefined.")
# NRF_BASE = os.path.abspath(os.environ["NRF_BASE"])

# if "HOMEKIT_OUTPUT" not in os.environ:
#     sys.exit("$HOMEKIT_OUTPUT environment variable undefined.")
# HOMEKIT_OUTPUT = os.path.abspath(os.environ["HOMEKIT_OUTPUT"])


# -- General configuration ------------------------------------------------

# Sphinx 2.0 changes the default from 'index' to 'contents'
master_doc = 'index'

# General information about the project.
project = 'HomeKit for nRF Connect SDK'
copyright = '2021, Nordic Semiconductor'
author = 'Nordic Semiconductor'

# Get rid of version number while keeping the spacing the same as for other
# docsets
version = '&nbsp;'

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
exclude_patterns = ['_build']

# -- Options for HTML output ----------------------------------------------

html_theme = "homekit"
html_theme_path = ['{}/doc/themes'.format(NRF_BASE)]
html_favicon = '{}/doc/static/images/favicon.ico'.format(NRF_BASE)

# The name for this set of Sphinx documents.  If None, it defaults to
# "<project> v<release> documentation".
html_title = "Apple HomeKit support for nRF Connect SDK"

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['{}/doc/static'.format(NRF_BASE)]

# If not '', a 'Last updated on:' timestamp is inserted at every page bottom,
# using the given strftime format.
html_last_updated_fmt = '%b %d, %Y'

# If false, no module index is generated.
html_domain_indices = False

# If false, no index is generated.
html_use_index = True

# If true, the index is split into individual pages for each letter.
html_split_index = True

# If true, "Created using Sphinx" is shown in the HTML footer. Default is True.
html_show_sphinx = False

# If true, "(C) Copyright ..." is shown in the HTML footer. Default is True.
html_show_copyright = True

# If true, license is shown in the HTML footer. Default is True.
html_show_license = True

def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/homekit.css")
    app.add_js_file("js/ncs.js")
