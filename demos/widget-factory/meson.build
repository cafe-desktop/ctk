
widgetfactory_sources = [
  'widget-factory.c'
]

widgetfactory_resources = gnome.compile_resources(
  'widgetfactory_resources',
  'widget-factory.gresource.xml',
  source_dir: '.'
)

foreach s: [ '16', '22', '24', '32', '48', '256']
  icon_destdir = join_paths(ctk_datadir, 'icons', 'hicolor', '@0@x@0@'.format(s), 'apps')
  icons = [join_paths('data', '@0@x@0@'.format(s), 'ctk3-widget-factory.png'),
           join_paths('data', '@0@x@0@'.format(s), 'ctk3-widget-factory-symbolic.symbolic.png')]
  install_data(icons, install_dir: icon_destdir)
endforeach

widget_factory = executable(
  'ctk3-widget-factory',
  widgetfactory_sources,
  widgetfactory_resources,
  dependencies: libctk_dep,
  include_directories : confinc,
  install: true
)

# desktop file
install_data('ctk3-widget-factory.desktop', install_dir: ctk_applicationsdir)
