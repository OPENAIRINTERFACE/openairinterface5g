cmake_minimum_required(VERSION 2.8)
set(PACKAGE_NAME "unitary_tests_simulators")
set(PHYSIM True)
set(RF_BOARD None)
set(XFORMS False)

set(DEBUG_PHY False)
set(MU_RECIEVER Flase)
set(RANDOM_BF False)
set(PBS_SIM False)
set(PERFECT_CE True)

set(MESSAGE_CHART_GENERATOR         False)
set(MESSAGE_CHART_GENERATOR_RLC_MAC False)
set(MESSAGE_CHART_GENERATOR_PHY     False)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../CMakeLists.txt)
