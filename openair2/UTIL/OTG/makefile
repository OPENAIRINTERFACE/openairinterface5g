
-D STANDALONE=1


CC = gcc 
  
OBJ_OTG = main.c otg_tx.c otg_tx_socket.c otg_rx.c ../MATH/oml.c ../MATH/taus.c otg.c otg_kpi.c 


help: 
	@echo ' "make clean" to clean the generated file'
	@echo ' "make otg" to compile OTG'
	
otg: ${OBJ_OTG} 
	${CC}  -g -O3 ${OBJ_OTG} -D STANDALONE=1 -lm -o otg 

clean: 
	@rm otg
