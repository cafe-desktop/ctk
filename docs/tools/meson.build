if x11_enabled
  doc_shooter_sources = [
    'shadow.c',
    'shooter.c',
    'widgets.c',
    '../../tests/ctkgears.c',
  ]

  doc_shooter = executable('doc-shooter', doc_shooter_sources,
                           include_directories: [ confinc, gdkinc, ctkinc, testinc, ],
                           dependencies: libctk_dep)
endif
