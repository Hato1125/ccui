add_rules('mode.debug', 'mode.release')

set_toolchains('gcc')

add_repositories('ccui-repo repo')
add_requires('thorvg v1.1.0', {
  configs = {
    engines = { 'gl' },
    loaders = { 'svg', 'ttf' },
    threads = true,
  }
})

target('ccui')
  set_kind('static')
  set_languages('c++26')
  add_files('src/**.cc')
  add_headerfiles('src/**.hh')
  add_includedirs('src', { public = true })
  add_packages('thorvg')
  add_cxxflags('-freflection', { force = true })
