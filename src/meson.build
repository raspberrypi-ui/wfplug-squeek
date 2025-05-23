gtkmm = dependency('gtkmm-3.0', version: '>=3.24')
wfpanel = dependency('wf-panel-pi')

wsources = files(
  'squeek.cpp'
)

wdeps = [ gtkmm, wfpanel ]

wargs = [ '-DPACKAGE_DATA_DIR="' + wresource_dir + '"', '-DGETTEXT_PACKAGE="wfplug_' + meson.project_name() +'"' ]

shared_module('lib' + meson.project_name(), wsources,
        dependencies: wdeps,
        install: true,
        install_dir: get_option('libdir') / 'wf-panel-pi',
        cpp_args : wargs,
        name_prefix: ''
)

metadata = files()
install_data(metadata, install_dir: metadata_dir)
