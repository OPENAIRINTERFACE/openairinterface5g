add_library(e2_ran_func_oran STATIC 
          ran_func_kpm.c
          ran_func_rc.c
          # For testing purposes 
  ../../flexric/test/rnd/fill_rnd_data_kpm.c             
  ../../flexric/test/rnd/fill_rnd_data_rc.c              
          )
