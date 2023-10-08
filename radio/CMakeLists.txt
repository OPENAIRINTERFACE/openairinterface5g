add_boolean_option(OAI_AW2SORI OFF "Activate OAI's AW2S driver" OFF)
if(OAI_AW2SORI)
  add_subdirectory(AW2SORI)
endif()

add_boolean_option(OAI_ETHERNET OFF "Activate OAI's Ethernet transport driver" OFF)
if(OAI_ETHERNET)
  add_subdirectory(ETHERNET)
endif()

add_boolean_option(OAI_BLADERF OFF "Activate OAI's BladeRF driver" OFF)
if(OAI_BLADERF)
  add_subdirectory(BLADERF)
endif()

add_boolean_option(OAI_FHI72 OFF "Activate OAI's FHI 7.2 (xran/fhi_lib) driver" OFF)
if(OAI_FHI72)
  add_subdirectory(fhi_72)
endif()

add_boolean_option(OAI_IRIS OFF "Activate OAI's IRIS/SoapySDR driver" OFF)
if(OAI_IRIS)
  add_subdirectory(IRIS)
endif()

add_boolean_option(OAI_LMSSDR OFF "Activate OAI's LimeSDR driver" OFF)
if(OAI_LMSSDR)
  add_subdirectory(LMSSDR)
endif()

add_boolean_option(OAI_SIMU ON "Activate OAI's rfsimulator driver" OFF)
if(OAI_SIMU)
  add_subdirectory(rfsimulator)
endif()

add_boolean_option(OAI_USRP OFF "Activate OAI's USRP driver" OFF)
if(OAI_USRP)
  add_subdirectory(USRP)
endif()
