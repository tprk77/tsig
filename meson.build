# meson.build

###########
# Project #
###########

project('tsig', 'cpp',
  version : '0.1.0',
  license : 'MIT',
  default_options : [
    'cpp_std=c++11',
    'buildtype=debugoptimized',
    'warning_level=3',
    'werror=true'
  ])

######################
# Declare Dependency #
######################

# TODO Remove fmt dependency...
fmt_dep = dependency('fmt', fallback : ['fmt', 'fmt_dep'])
tsig_dep = declare_dependency(
  version : meson.project_version(),
  include_directories : include_directories('.'),
  dependencies : [fmt_dep])

#########################
# Tests, Utilities, etc #
#########################

# Tests
gtest_dep = dependency('gtest', main : false, fallback : ['gtest', 'gtest_dep'])
signal_test = executable(
  'signal_test', 'tests/signal_test.cpp', dependencies : [tsig_dep, gtest_dep])
test('signal_test', signal_test)

# Examples
node_example = executable(
  'node_example', 'examples/node_example.cpp', dependencies : [tsig_dep])

###########
# Install #
###########

headers = [
  'tsig/signal.hpp',
]

install_headers(headers, subdir : 'tsig')

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name : 'tsig',
  filebase : 'tsig',
  description : 'Header-only lightweight signals',
  subdirs : 'tsig',
  version : meson.project_version())
