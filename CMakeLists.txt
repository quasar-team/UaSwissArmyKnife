add_definitions( -std=gnu++0x )

set( UASDK "/opt/OpcUaToolkit-static-1.5.5" )

include_directories(
	${UASDK}/include/uabase
	${UASDK}/include/uaclient
	${UASDK}/include/uastack
)

link_directories( ${UASDK}/lib )

add_executable(subscribe_to_all
  main.cpp
  )



target_link_libraries( subscribe_to_all -luaclient -luabase -luapki -lxmlparser -luastack -lcrypto -lxml2 -lboost_program_options-mt -lpthread  )
