add_boolean_option(OAI_BENETEL4G OFF "Activate OAI's Benetel 4G radio driver" OFF)
if(OAI_BENETEL4G)
  add_subdirectory(4g)
endif()

add_boolean_option(OAI_BENETEL5G OFF "Activate OAI's Benetel 5G radio driver" OFF)
if(OAI_BENETEL5G)
  add_subdirectory(5g)
endif()
