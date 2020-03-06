### LDPC DECODER 
+ `intuitive_ldpc`: 
	Intuitive methods of LDPC decoder. This method stores the whole expanded Base Graph, CN, BN buffer as they are. Then, does the cn and bn process according to the equation in `nrLDPC.pdf`.

+ `optimized_ldpc`: 
	This version inherits the idea of current oai ldpc_decoder. It stores lots of LUTs and shrinks the cn, bnbuffer size comparing to the former implementation. Right now, it only supports the longest code block 8448.

### Usage
+ `make ldpc` will compile the program `ldpc`.
+ `make prof [num=<0~100>]` will show the detail of GPU activity.
+ `make test` will build ldpc executable and run `check.sh` to verify the correctness of the implementation.

### Verification
+ The input (channel output) of the LDPC decoder is produced by the `ldpctest` program from OAI.
+ The verification is done by comparing the data decoded with the input data (channel output), and also the data output(`estimated_output`) produced by oai `ldpctest`.


