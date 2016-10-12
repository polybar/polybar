install(FILES config
  DESTINATION share/examples/lemonbuddy
  COMPONENT config)

install(FILES config.bspwm
  DESTINATION share/examples/lemonbuddy
  COMPONENT config)

if(ENABLE_I3)
  install(FILES config.i3
    DESTINATION share/examples/lemonbuddy
    COMPONENT config)
endif()
