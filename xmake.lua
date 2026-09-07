add_rules('mode.debug', 'mode.release')

set_toolchains('gcc')

target('ccui')
  set_kind('static')
  set_languages('c++26')
  add_files('src/**.cc')
  add_cxxflags('-freflection', { force = true })
