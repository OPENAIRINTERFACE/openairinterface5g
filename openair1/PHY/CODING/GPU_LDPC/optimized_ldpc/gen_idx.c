#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef test
#include "../BGs/isip_ldpc_bg1_i1.h"
#endif
int l2c_idx[316*384] = {};		// cnbuf[ tid ] = llr[ l2c_idx[tid] ]
int l2b_idx[316*384] = {};		// bnbuf[ tid ] = llr[ l2b_idx[tid] ]
int llr_idx[26113] = {};		// bnbuf2llr, start = llr_idx[tid], end = llr_idx[tid+1], llrbuf[tid] = sum of (bnbuf[start] to bnbuf[end])
int c2b_idx[316*384] = {};    	// bnbuf[ tid ] = cnbuf[ c2b_idx[tid] ]
int b2c_idx[316*384] = {};		// cnbuf[ tid ] = bnbuf[ b2c_idx[tid] ]
int cnproc_start_idx[316*384] = {};	// index for cnproc, int start = cnproc_start_idx[tid]
int cnproc_end_idx[316*384] = {};	// index for cnproc, int end = cnproc_end_idx[tid]
int bnproc_start_idx[316*384] = {};	// index for bnproc, int start = bnproc_start_idx[tid]
int bnproc_end_idx[316*384] = {};	// index for bnproc, int end = bnproc_end_idx[tid]

int *matrix_transpose(int *matrix, int row, int col)
{

}

void write_to_file(FILE *fp, const char *arr_name, int *arr, int size)
{
	fprintf(fp, "%s[%d] = { %d", arr_name, size, arr[0]);
	for(int i = 1; i < size; i++){
		fprintf(fp, ", %d", arr[i]);
	}
	fprintf(fp, "};\n");
}

void matrix_expansion(const int *BG, int row, int col, int Zc, int *matrix)
{
	for(int i = 0; i < row; i++){
		for(int j = 0; j < col; j++){
			int val = BG[i * col + j];
			if(val != 0){
				val = (val-1)%Zc;
				for(int k = i*Zc; k < (i+1)*Zc; k++){
					int idx = k*(col*Zc) + (j*Zc+val);
					matrix[idx] = 1;
					val = (val+1)%Zc;
				}
			}
		}
	}
}

void print_matrix(const int *M, int row, int col)
{
	printf("Matrix:\n{\n");
	for(int i = 0; i < row; i++){
		printf("\t");
		for(int j = 0; j < col; j++){
			printf(" %d,", M[i*col + j]);
		}
		printf("\n");
	}
	printf("}\n");
}


void build_index(const int *BG, int row, int col, int Zc)
{
	int *matrix = (int*)malloc(row*col*Zc*Zc*sizeof(int));
	int cnidx = 0, bnidx = 0, aidx = 0, pidx = 0, lidx1 = 0, lidx2 = 0;
	
	matrix_expansion(BG, row, col, Zc, matrix);

	// cn label & l2c_idx
	for(int i = 0; i < row*Zc; i++){
		for(int j = 0; j < col*Zc; j++){
			int k = i*col*Zc + j;
			if(matrix[k] == 1){
				matrix[k] = cnidx+1;
				l2c_idx[cnidx] = j;
				cnidx++;
			}
		}
	}

	// build c2b_idx & bnproc_idx
	for(int i = 0; i < col*Zc; i++){
		int cnt = 0;
		int start = aidx, end = 0;
		for(int j = 0; j < row*Zc; j++){
			int val = matrix[i + j*col*Zc];
			if(val != 0){
				c2b_idx[aidx] = val -1;
				aidx++, cnt++;
			}
		}
		end = start + cnt;
		while(cnt--){
			bnproc_start_idx[pidx] = start;
			bnproc_end_idx[pidx] = end;
			pidx++;
		}
	}

	printf("cnidx %d\naidx %d\npidx %d\n", cnidx, aidx, pidx);

	matrix_expansion(BG, row, col, Zc, matrix);

	// bn label & llr_idx
	for(int i = 0; i < col*Zc; i++){
		llr_idx[lidx1++] = bnidx;
		for(int j = 0; j < row*Zc; j++){
			int k = i + j*col*Zc;
			if(matrix[k] == 1){
				matrix[k] = bnidx+1;
				l2b_idx[lidx2++] = i;
				bnidx++;
			}
		}
	}
	llr_idx[lidx1] = bnidx;

	// build b2c_idx & cnproc_idx
	aidx = pidx = 0;
	for(int i = 0; i < row*Zc; i++){
		int cnt = 0;
		int start = aidx, end = 0;
		for(int j = 0; j < col*Zc; j++){
			int val = matrix[i*col*Zc + j];
			if(val != 0){
				b2c_idx[aidx] = val - 1;
				aidx++, cnt++;
			}
		}
		end = start + cnt;
		while(cnt--){
			cnproc_start_idx[pidx] = start;
			cnproc_end_idx[pidx] = end;
			pidx++;
		}
	}	
	
	printf("bnidx %d\naidx %d\npidx %d\n", bnidx, aidx, pidx);
	printf("lidx1 %d\nlidx2 %d\n", lidx1, lidx2);

	free(matrix);
}

void generate_header(const char *file, int col, int entry, int Zc)
{
#ifdef test
	FILE *f = fopen("test.h", "w");
#else
	FILE *f = fopen(file, "w");
#endif
	write_to_file(f, "int l2c_idx", l2c_idx, entry*Zc);
	write_to_file(f, "int c2b_idx", c2b_idx, entry*Zc);
	write_to_file(f, "int b2c_idx", b2c_idx, entry*Zc);
	write_to_file(f, "int cnproc_start_idx", cnproc_start_idx, entry*Zc);
	write_to_file(f, "int cnproc_end_idx", cnproc_end_idx, entry*Zc);
	write_to_file(f, "int bnproc_start_idx", bnproc_start_idx, entry*Zc);
	write_to_file(f, "int bnproc_end_idx", bnproc_end_idx, entry*Zc);
	write_to_file(f, "int llr_idx", llr_idx, col*Zc+1);
	write_to_file(f, "int l2b_idx", l2b_idx, entry*Zc);

	fclose(f);
}


int main(int argc, char** argv)
{
	int test_BG[4*5] ={ 2, 0, 1, 2, 0,   
    					0, 3, 0, 2, 2, 
						1, 0, 3, 3, 0, 
				   		0, 3, 0, 0, 1};
	// default
    int idx = 0, Zc = 384, BG = 1;
    int max_row = 46, max_col = 68;
	int entry = 316;
	char *header_file = "bg1_i1_index_array.h";
#ifdef test
	if(argc == 1){
		printf("\nusage: %s [BG_row] [BG_col] [Zc] [entry] [output_header]\n\n", argv[0]);
		return -1;
	}
#endif
    if(argc == 6){
        max_row = atoi(argv[1]);
        max_col = atoi(argv[2]);
        Zc      = atoi(argv[3]);
		entry	= atoi(argv[4]);
		header_file	= argv[5];
		printf("max_row %d, max_col %d, Zc %d\n", max_row, max_col, Zc);
    }
#ifdef test
	build_index(test_BG, max_row, max_col, Zc);
#else
	build_index(BG1_I1, max_row, max_col, Zc);
#endif
	generate_header(header_file, max_col, entry, Zc);

    return 0;
}
