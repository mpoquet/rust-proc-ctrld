project('daemon', ['c', 'cpp'],
  default_options : [
    'cpp_std=c++17',
    'c_std=gnu11'
  ]
)

# Set include directories correctly
inc_dir = include_directories([
  'include' 
])

# Dependencies
thread_dep = dependency('threads')
flatbuffers_dep = dependency('flatbuffers', required : true)

# Sources C++
cpp_sources = files(
  'daemon-features/serialize.cpp',
  'daemon-features/serialize_wrapper.cpp'
)

# Sources C
c_sources = files(
  'daemon-features/clone.c',
  'daemon-features/Errors.c',
  'daemon-features/events.c',
  'daemon-features/Network.c',
  'daemon-features/process_manager.c',
  'daemon-features/Detect_socket.c', 
  'daemon-core.c'
)

# Test sources
test_sources = files('test/Regression_test.c')

# Common args
cpp_args = ['-DFLATBUFFERS_DEBUG_VERIFICATION_FAILURE']

# Compilation du daemon
daemon = executable('daemon',
  sources: [c_sources, cpp_sources],
  include_directories: inc_dir,
  dependencies: [flatbuffers_dep, thread_dep],
  cpp_args: cpp_args,
  install: true
)

# Tests 
regression = executable('regression_tests',
  sources: [
    'daemon-features/clone.c',
    'daemon-features/Network.c',
    cpp_sources,
    test_sources
  ],
  include_directories: inc_dir,
  dependencies: [flatbuffers_dep, thread_dep],
  cpp_args: cpp_args
)

test('regression tests', regression)