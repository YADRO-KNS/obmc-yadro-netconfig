# Rules for building with Meson

project(
  'netconfig',
  'cpp',
  default_options: [
   'warning_level=3',
   'werror=true',
   'cpp_std=c++17',
  ],
  license: 'Apache-2.0',
)

version = vcs_tag(command: [ 'git', 'describe', '--always', '--dirty', '--long' ],
                  input: 'src/version.hpp.in',
                  output: 'version.hpp')

conf = configuration_data()
conf.set_quoted('DEFAULT_NETIFACE', get_option('default-netiface'))
configure_file(output: 'config.hpp', configuration: conf)

build_tests = get_option('tests')
subdir('test')

executable(
  'netconfig',
  [
    version,
    'src/arguments.cpp',
    'src/dbus.cpp',
    'src/main.cpp',
    'src/netconfig.cpp',
    'src/show.cpp',
  ],
  dependencies: [
    dependency('sdbusplus'),
  ],
  install: true
)
