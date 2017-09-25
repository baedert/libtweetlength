project('libtweetlength', 'c')

add_project_arguments('-DG_LOG_DOMAIN="libtl"', language: 'c')

glib_dep = dependency('glib-2.0')

sources = files([
  'src/libtweetlength.c'
])

headers = files([
  'src/libtweetlength.h'
])
install_headers(headers)

enable_debug = get_option('buildtype') == 'debug' or get_option('buildtype') == 'debugoptimized'
if enable_debug
  add_project_arguments('-DDEBUG', language: 'c')
endif


libtl = library(
  'tweetlength',
  sources,
  dependencies: glib_dep
)

libtl_dep = declare_dependency(
  link_with: libtl,
  dependencies: glib_dep,
  include_directories: include_directories('src')
)

subdir('tests')
