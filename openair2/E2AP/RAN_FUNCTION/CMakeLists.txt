add_subdirectory(CUSTOMIZED)
add_subdirectory(O-RAN)

add_library(e2_ran_func STATIC 
          init_ran_func.c
          )

target_link_libraries(e2_ran_func   
                       PUBLIC  
                       e2_ran_func_cust 
                       e2_ran_func_oran 
                       ) 
