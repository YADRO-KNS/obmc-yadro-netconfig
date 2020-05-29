# Rules for building tests

test(
  'netconfig',
  executable(
    'netconfig_test',
    [
      'arguments_test.cpp',
      '../src/arguments.cpp',
    ],
    dependencies: [
      dependency('gtest', main: true, disabler: true, required: build_tests),
    ],
    include_directories: '../src',
  )
)
