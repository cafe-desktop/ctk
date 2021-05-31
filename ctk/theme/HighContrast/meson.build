hc_scss_files = files([
  '_colors-hc.scss'
])

theme_variants = [
  'inverse',
]

theme_deps += custom_target('HighContrast',
                            input: 'ctk-contained.scss',
                            output: 'ctk-contained.css',
                            command: [
                              sassc, sassc_opts, '@INPUT@', '@OUTPUT@',
                            ],
                            depend_files: scss_files,
                            build_by_default: true)

foreach variant: theme_variants
  theme_deps += custom_target('HighContrast-' + variant,
                              input: 'ctk-contained-@0@.scss'.format(variant),
                              output: 'ctk-contained-@0@.css'.format(variant),
                              command: [
                                sassc, sassc_opts, '@INPUT@', '@OUTPUT@',
                              ],
                              depend_files: scss_files,
                              build_by_default: true)
endforeach
