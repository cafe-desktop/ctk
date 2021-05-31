
## These should be in the order you want them to appear in the
## demo app, which means alphabetized by demo title, not filename
demos = files([
  'application_demo.c',
  'assistant.c',
  'builder.c',
  'button_box.c',
  'changedisplay.c',
  'clipboard.c',
  'colorsel.c',
  'combobox.c',
  'css_accordion.c',
  'css_basics.c',
  'css_blendmodes.c',
  'css_multiplebgs.c',
  'css_pixbufs.c',
  'css_shadows.c',
  'cursors.c',
  'dialog.c',
  'drawingarea.c',
  'editable_cells.c',
  'entry_buffer.c',
  'entry_completion.c',
  'event_axes.c',
  'expander.c',
  'filtermodel.c',
  'fishbowl.c',
  'foreigndrawing.c',
  'gestures.c',
  'glarea.c',
  'headerbar.c',
  'hypertext.c',
  'iconview.c',
  'iconview_edit.c',
  'images.c',
  'infobar.c',
  'links.c',
  'listbox.c',
  'flowbox.c',
  'list_store.c',
  'markup.c',
  'menus.c',
  'modelbutton.c',
  'offscreen_window.c',
  'offscreen_window2.c',
  'overlay.c',
  'overlay2.c',
  'paint.c',
  'panes.c',
  'pickers.c',
  'pixbufs.c',
  'popover.c',
  'printing.c',
  'revealer.c',
  'rotated_text.c',
  'scale.c',
  'search_entry.c',
  'search_entry2.c',
  'shortcuts.c',
  'sidebar.c',
  'sizegroup.c',
  'spinbutton.c',
  'spinner.c',
  'stack.c',
  'tabs.c',
  'textmask.c',
  'textview.c',
  'textscroll.c',
  'theming_style_classes.c',
  'toolpalette.c',
  'transparent.c',
  'tree_store.c',
])


ctkdemo_deps = [libctk_dep]

if harfbuzz_dep.found() and pangoft_dep.found()
  demos += files('font_features.c')
  ctkdemo_deps += [harfbuzz_dep, pangoft_dep]
endif

if os_unix
  demos += files('pagesetup.c')
endif

ctkdemo_sources = demos + files([
  'ctkfishbowl.c',
  'ctkfishbowl.h',
  'main.c',
])


geninclude = find_program('geninclude.py')
demos_h = custom_target(
  'ctk3 demo header',
  output  : 'demos.h',
  input   : demos,
  command : [geninclude, '@OUTPUT@', '@INPUT@'],
)

ctkdemo_resources = gnome.compile_resources(
  'ctkdemo_resources',
  'demo.gresource.xml',
  source_dir: '.'
)

ctkdemo = executable(
  'ctk3-demo',
  ctkdemo_sources,
  demos_h,
  ctkdemo_resources,
  dependencies: ctkdemo_deps,
  include_directories : confinc,
  install: true
)

ctkapplicationdemo = executable(
  'ctk3-demo-application',
  'application.c',
  demos_h,
  ctkdemo_resources,
  dependencies: ctkdemo_deps,
  include_directories : confinc,
  install: true
)

ctk_settings_schemas = [
  'org.ctk.Settings.FileChooser.gschema.xml',
  'org.ctk.Settings.ColorChooser.gschema.xml',
  'org.ctk.Settings.EmojiChooser.gschema.xml',
  'org.ctk.Settings.Debug.gschema.xml',
]

foreach s: [ '16', '22', '24', '32', '48', '256']
  icon_destdir = join_paths(ctk_datadir, 'icons', 'hicolor', '@0@x@0@'.format(s), 'apps')
  icons = [join_paths('data', '@0@x@0@'.format(s), 'ctk3-demo.png'),
           join_paths('data', '@0@x@0@'.format(s), 'ctk3-demo-symbolic.symbolic.png')]
  install_data(icons, install_dir: icon_destdir)
endforeach

# desktop file
install_data('ctk3-demo.desktop', install_dir: ctk_applicationsdir)

# GSettings
install_data('org.ctk.Demo.gschema.xml', install_dir: ctk_schemasdir)
gnome.compile_schemas(depend_files: files(['org.ctk.Demo.gschema.xml']))
