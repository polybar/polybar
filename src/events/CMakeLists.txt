make_library(polybar-events
  STATIC
  INTERNAL
  SOURCES
    signal_emitter.cpp
    signal_receiver.cpp
  INCLUDE_DIRS
    ${dirs})
