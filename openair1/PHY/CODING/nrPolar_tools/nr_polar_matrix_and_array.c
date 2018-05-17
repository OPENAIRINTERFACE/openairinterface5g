#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"

void nr_matrix_multiplication_uint8_t_1D_uint8_t_2D(uint8_t *matrix1, uint8_t **matrix2,
		uint8_t *output, uint16_t row, uint16_t col) {

	for (uint16_t i = 0; i < col; i++) {
		output[i] = 0;
		for (uint16_t j = 0; j < row; j++) {
			output[i] += matrix1[j] * matrix2[j][i];
		}
	}
}

uint8_t ***nr_alloc_uint8_t_3D_array(uint16_t xlen, uint16_t ylen, uint16_t zlen) {
	uint8_t ***output;
	int i, j;

	if ((output = malloc(xlen * sizeof(*output))) == NULL) {
		perror("[nr_alloc_uint8_t_3D_array] Problem at 1D allocation");
		return NULL;
	}
	for (i = 0; i < xlen; i++)
		output[i] = NULL;


	for (i = 0; i < xlen; i++)
		if ((output[i] = malloc(ylen * sizeof *output[i])) == NULL) {
			perror("[nr_alloc_uint8_t_3D_array] Problem at 2D allocation");
			nr_free_uint8_t_3D_array(output, xlen, ylen);
			return NULL;
		}
	for (i = 0; i < xlen; i++)
		for (j = 0; j < ylen; j++)
			output[i][j] = NULL;


	for (i = 0; i < xlen; i++)
		for (j = 0; j < ylen; j++)
			if ((output[i][j] = malloc(zlen * sizeof *output[i][j])) == NULL) {
				perror("[nr_alloc_uint8_t_3D_array] Problem at 3D allocation");
				nr_free_uint8_t_3D_array(output, xlen, ylen);
				return NULL;
			}

	return output;
}

double ***nr_alloc_double_3D_array(uint16_t xlen, uint16_t ylen, uint16_t zlen) {
	double ***output;
	int i, j;

	if ((output = malloc(xlen * sizeof(*output))) == NULL) {
		perror("[nr_alloc_double_3D_array] Problem at 1D allocation");
		return NULL;
	}
	for (i = 0; i < xlen; i++)
		output[i] = NULL;


	for (i = 0; i < xlen; i++)
		if ((output[i] = malloc(ylen * sizeof *output[i])) == NULL) {
			perror("[nr_alloc_double_3D_array] Problem at 2D allocation");
			nr_free_double_3D_array(output, xlen, ylen);
			return NULL;
		}
	for (i = 0; i < xlen; i++)
		for (j = 0; j < ylen; j++)
			output[i][j] = NULL;


	for (i = 0; i < xlen; i++)
		for (j = 0; j < ylen; j++)
			if ((output[i][j] = malloc(zlen * sizeof *output[i][j])) == NULL) {
				perror("[nr_alloc_double_3D_array] Problem at 3D allocation");
				nr_free_double_3D_array(output, xlen, ylen);
				return NULL;
			}

	return output;
}

uint8_t **nr_alloc_uint8_t_2D_array(uint16_t xlen, uint16_t ylen) {
	uint8_t **output;
	int i, j;

	if ((output = malloc(xlen * sizeof(*output))) == NULL) {
		perror("[nr_alloc_uint8_t_2D_array] Problem at 1D allocation");
		return NULL;
	}
	for (i = 0; i < xlen; i++)
		output[i] = NULL;


	for (i = 0; i < xlen; i++)
		if ((output[i] = malloc(ylen * sizeof *output[i])) == NULL) {
			perror("[nr_alloc_uint8_t_2D_array] Problem at 2D allocation");
			nr_free_uint8_t_2D_array(output, xlen);
			return NULL;
		}
	for (i = 0; i < xlen; i++)
		for (j = 0; j < ylen; j++)
			output[i][j] = 0;

	return output;
}

void nr_free_uint8_t_3D_array(uint8_t ***input, uint16_t xlen, uint16_t ylen) {
	int i, j;

	for (i = 0; i < xlen; i++) {
		for (j = 0; j < ylen; j++) {
			free(input[i][j]);
		}
		free(input[i]);
	}
	free(input);
}

void nr_free_uint8_t_2D_array(uint8_t **input, uint16_t xlen) {
	for (int i = 0; i < xlen; i++) free(input[i]);
	free(input);
}

void nr_free_double_3D_array(double ***input, uint16_t xlen, uint16_t ylen) {
	int i, j;

	for (i = 0; i < xlen; i++) {
		for (j = 0; j < ylen; j++) {
			free(input[i][j]);
		}
		free(input[i]);
	}
	free(input);
}

// Modified Bubble Sort.
void nr_sort_asc_double_1D_array_ind(double *matrix, uint8_t *ind, uint8_t len) {
	uint8_t swaps;
	double temp;
	uint8_t tempInd;

	for (uint8_t i = 0; i < len; i++) {
		swaps = 0;
		for (uint8_t j = 0; j < (len - i) - 1; j++) {
			if (matrix[j] > matrix[j + 1]) {
				temp = matrix[j];
				matrix[j] = matrix[j + 1];
				matrix[j + 1] = temp;

				tempInd = ind[j];
				ind[j] = ind[j + 1];
				ind[j + 1] = tempInd;

				swaps++;
			}
		}
		if (swaps == 0)
			break;
	}
}
