add_rules('mode.debug', 'mode.release')


target('neko')
  set_kind('static')
  set_languages('c++26')
  add_files('src/**.cc')
  add_cxxflags('-freflection', { force = true })
