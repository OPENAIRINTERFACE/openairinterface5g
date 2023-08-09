/**********************************************************************
* FILENAME    :  input_buffer_test
*
* MODULE      :  test of UE synchronisation
*
* DESCRIPTION :  it allows unitary tests of UE synchronisation on host machine
*
 ************************************************************************/

#ifndef INPUT_BUFFER_H 
#define INPUT_BUFFER_H 

#ifdef DEFINE_VARIABLES_INPUT_BUFFER_TEST_H
#define EXTERNAL 
#define INIT_VARIABLES_INPUT_BUFFER_H 
#else 
#define EXTERNAL  extern 
#undef INIT_VARIABLES_INPUT_BUFFER_H 
#endif 

#ifndef INIT_VARIABLES_INPUT_BUFFER_H
EXTERNAL short input_buffer[]
#else
EXTERNAL short input_buffer[307360] = {0}
#endif /* INIT_VARIABLES_INPUT_BUFFER_H */ 
;
#undef EXTERNAL
#undef INIT_VARIABLES_INPUT_BUFFER_H
#endif /* INPUT_BUFFER_H */ 
