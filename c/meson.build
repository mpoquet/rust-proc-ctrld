project('daemon', 'c',)

src_dir = 'daemon-features'
sources = files(
  'daemon-features/clone.c',
  'daemon-features/Errors.c',
  'daemon-features/events.c',
  'daemon-features/Detect_socket.c',
  'daemon-features/Network.c',
  'daemon-features/process_manager.c',
  'daemon-core.c'
)

executable('daemon', sources,
  include_directories : include_directories(src_dir),
  install : true
)