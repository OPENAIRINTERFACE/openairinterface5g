/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*!\file nrLDPC_decoder_offload.c
 * \brief Defines the LDPC decoder
 * \author openairinterface 
 * \date 12-06-2021
 * \version 1.0
 * \note
 * \warning
 */


#include <stdint.h>
#include <immintrin.h>
#include "nrLDPCdecoder_defs.h"
#include "nrLDPC_types.h"
#include "nrLDPC_init.h"
#include "nrLDPC_mPass.h"
#include "nrLDPC_cnProc.h"
#include "nrLDPC_bnProc.h"

#define NR_LDPC_ENABLE_PARITY_CHECK
//#define NR_LDPC_PROFILER_DETAIL

#ifdef NR_LDPC_DEBUG_MODE
#include "nrLDPC_tools/nrLDPC_debug.h"
#endif

#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <rte_eal.h>
#include <rte_common.h>
#include <rte_string_fns.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include "nrLDPC_offload.h"


#include <math.h>

#include <rte_dev.h>
#include <rte_launch.h>
#include <rte_bbdev.h>
#include <rte_malloc.h>
#include <rte_random.h>
#include <rte_hexdump.h>
#include <rte_interrupts.h>
#include "../../../targets/ARCH/test-bbdev/main.h"
#include "../../../targets/ARCH/test-bbdev/test_bbdev_vector.h"

#define MAX_QUEUES RTE_MAX_LCORE
#define TEST_REPETITIONS 1000
/* Switch between PMD and Interrupt for throughput TC */
static bool intr_enabled;
static struct test_bbdev_vector test_vector;
/* LLR arithmetic representation for numerical conversion */
static int ldpc_llr_decimals;
static int ldpc_llr_size;
/* Keep track of the LDPC decoder device capability flag */
static uint32_t ldpc_cap_flags;

/* Represents tested active devices */
static struct active_device {
	const char *driver_name;
	uint8_t dev_id;
	uint16_t supported_ops;
	uint16_t queue_ids[MAX_QUEUES];
	uint16_t nb_queues;
	struct rte_mempool *ops_mempool;
	struct rte_mempool *in_mbuf_pool;
	struct rte_mempool *hard_out_mbuf_pool;
	struct rte_mempool *soft_out_mbuf_pool;
	struct rte_mempool *harq_in_mbuf_pool;
	struct rte_mempool *harq_out_mbuf_pool;
} active_devs[RTE_BBDEV_MAX_DEVS];

static uint8_t nb_active_devs;

/* Data buffers used by BBDEV ops */
struct test_buffers {
	struct rte_bbdev_op_data *inputs;
	struct rte_bbdev_op_data *hard_outputs;
	struct rte_bbdev_op_data *soft_outputs;
	struct rte_bbdev_op_data *harq_inputs;
	struct rte_bbdev_op_data *harq_outputs;
};

/* Operation parameters specific for given test case */
struct test_op_params {
	struct rte_mempool *mp;
	struct rte_bbdev_dec_op *ref_dec_op;
	struct rte_bbdev_enc_op *ref_enc_op;
	uint16_t burst_sz;
	uint16_t num_to_process;
	uint16_t num_lcores;
	int vector_mask;
	rte_atomic16_t sync;
	struct test_buffers q_bufs[RTE_MAX_NUMA_NODES][MAX_QUEUES];
};

/* Contains per lcore params */
struct thread_params {
	uint8_t dev_id;
	uint16_t queue_id;
	uint32_t lcore_id;
	uint64_t start_time;
	double ops_per_sec;
	double mbps;
	uint8_t iter_count;
	double iter_average;
	double bler;
	rte_atomic16_t nb_dequeued;
	rte_atomic16_t processing_status;
	rte_atomic16_t burst_sz;
	struct test_op_params *op_params;
	struct rte_bbdev_dec_op *dec_ops[MAX_BURST];
	struct rte_bbdev_enc_op *enc_ops[MAX_BURST];
};

/* Defines how many testcases can be specified as cmdline args */
#define MAX_CMDLINE_TESTCASES 8

static const char tc_sep = ',';

/* Declare structure for command line test parameters and options */
static struct test_params {
	struct test_command *test_to_run[MAX_CMDLINE_TESTCASES];
	unsigned int num_tests;
	unsigned int num_ops;
	unsigned int burst_sz;
	unsigned int num_lcores;
	double snr;
	unsigned int iter_max;
	char test_vector_filename[PATH_MAX];
	bool init_device;
} test_params;

static struct test_commands_list commands_list =
	TAILQ_HEAD_INITIALIZER(commands_list);

void
add_test_command(struct test_command *t)
{
	TAILQ_INSERT_TAIL(&commands_list, t, next);
}
int
unit_test_suite_runner(struct unit_test_suite *suite)
{
	int test_result = TEST_SUCCESS;
	unsigned int total = 0, skipped = 0, succeeded = 0, failed = 0;
	uint64_t start, end;

	printf("\n===========================================================\n");
	printf("Starting Test Suite : %s\n", suite->suite_name);

	start = rte_rdtsc_precise();

	if (suite->setup) {
		test_result = suite->setup();
		if (test_result == TEST_FAILED) {
			printf(" + Test suite setup %s failed!\n",
					suite->suite_name);
			printf(" + ------------------------------------------------------- +\n");
			return 1;
		}
		if (test_result == TEST_SKIPPED) {
			printf(" + Test suite setup %s skipped!\n",
					suite->suite_name);
			printf(" + ------------------------------------------------------- +\n");
			return 0;
		}
	}

	while (suite->unit_test_cases[total].testcase) {
		if (suite->unit_test_cases[total].setup)
			test_result = suite->unit_test_cases[total].setup();

		if (test_result == TEST_SUCCESS)
			test_result = suite->unit_test_cases[total].testcase();

		if (suite->unit_test_cases[total].teardown)
			suite->unit_test_cases[total].teardown();

		if (test_result == TEST_SUCCESS) {
			succeeded++;
			printf("TestCase [%2d] : %s passed\n", total,
					suite->unit_test_cases[total].name);
		} else if (test_result == TEST_SKIPPED) {
			skipped++;
			printf("TestCase [%2d] : %s skipped\n", total,
					suite->unit_test_cases[total].name);
		} else {
			failed++;
			printf("TestCase [%2d] : %s failed\n", total,
					suite->unit_test_cases[total].name);
		}

		total++;
	}

	/* Run test suite teardown */
	if (suite->teardown)
		suite->teardown();

	end = rte_rdtsc_precise();

	printf(" + ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ +\n");
	printf(" + Test Suite Summary : %s\n", suite->suite_name);
	printf(" + Tests Total :       %2d\n", total);
	printf(" + Tests Skipped :     %2d\n", skipped);
	printf(" + Tests Passed :      %2d\n", succeeded);
	printf(" + Tests Failed :      %2d\n", failed);
	printf(" + Tests Lasted :       %lg ms\n",
			((end - start) * 1000) / (double)rte_get_tsc_hz());
	printf(" + ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ +\n");

	return (failed > 0) ? 1 : 0;
}
const char *
get_vector_filename(void)
{
	return test_params.test_vector_filename;
}
unsigned int
get_num_ops(void)
{
	return test_params.num_ops;
}

unsigned int
get_burst_sz(void)
{
	return test_params.burst_sz;
}

unsigned int
get_num_lcores(void)
{
	return test_params.num_lcores;
}

double
get_snr(void)
{
	return test_params.snr;
}

unsigned int
get_iter_max(void)
{
	return test_params.iter_max;
}

bool
get_init_device(void)
{
	return test_params.init_device;
}
static void
print_usage(const char *prog_name)
{
	struct test_command *t;

	printf("***Usage: %s [EAL params] [-- [-n/--num-ops NUM_OPS]\n"
			"\t[-b/--burst-size BURST_SIZE]\n"
			"\t[-v/--test-vector VECTOR_FILE]\n"
			"\t[-c/--test-cases TEST_CASE[,TEST_CASE,...]]]\n",
			prog_name);

	printf("Available testcases: ");
	TAILQ_FOREACH(t, &commands_list, next)
		printf("%s ", t->command);
	printf("\n");
}

static int
parse_args(int argc, char **argv, struct test_params *tp)
{
	int opt, option_index;
	unsigned int num_tests = 0;
	bool test_cases_present = false;
	bool test_vector_present = false;
	struct test_command *t;
	char *tokens[MAX_CMDLINE_TESTCASES];
	int tc, ret;

	static struct option lgopts[] = {
		{ "num-ops", 1, 0, 'n' },
		{ "burst-size", 1, 0, 'b' },
		{ "test-cases", 1, 0, 'c' },
		{ "test-vector", 1, 0, 'v' },
		{ "lcores", 1, 0, 'l' },
		{ "snr", 1, 0, 's' },
		{ "iter_max", 6, 0, 't' },
		{ "init-device", 0, 0, 'i'},
		{ "help", 0, 0, 'h' },
		{ NULL,  0, 0, 0 }
	};
	tp->iter_max = DEFAULT_ITER;

	while ((opt = getopt_long(argc, argv, "hin:b:c:v:l:s:t:", lgopts,
			&option_index)) != EOF)
		switch (opt) {
		case 'n':
			TEST_ASSERT(strlen(optarg) > 0,
					"Num of operations is not provided");
			tp->num_ops = strtol(optarg, NULL, 10);
			break;
		case 'b':
			TEST_ASSERT(strlen(optarg) > 0,
					"Burst size is not provided");
			tp->burst_sz = strtol(optarg, NULL, 10);
			TEST_ASSERT(tp->burst_sz <= MAX_BURST,
					"Burst size mustn't be greater than %u",
					MAX_BURST);
			break;
		case 'c':
			TEST_ASSERT(test_cases_present == false,
					"Test cases provided more than once");
			test_cases_present = true;

			ret = rte_strsplit(optarg, strlen(optarg),
					tokens, MAX_CMDLINE_TESTCASES, tc_sep);

			TEST_ASSERT(ret <= MAX_CMDLINE_TESTCASES,
					"Too many test cases (max=%d)",
					MAX_CMDLINE_TESTCASES);

			for (tc = 0; tc < ret; ++tc) {
				/* Find matching test case */
				TAILQ_FOREACH(t, &commands_list, next)
					if (!strcmp(tokens[tc], t->command))
						tp->test_to_run[num_tests] = t;

				TEST_ASSERT(tp->test_to_run[num_tests] != NULL,
						"Unknown test case: %s",
						tokens[tc]);
				++num_tests;
			}
			break;
		case 'v':
			TEST_ASSERT(test_vector_present == false,
					"Test vector provided more than once");
			test_vector_present = true;

			TEST_ASSERT(strlen(optarg) > 0,
					"Config file name is null");

			snprintf(tp->test_vector_filename,
					sizeof(tp->test_vector_filename),
					"%s", optarg);
			break;
		case 's':
			TEST_ASSERT(strlen(optarg) > 0,
					"SNR is not provided");
			tp->snr = strtod(optarg, NULL);
			break;
		case 't':
			TEST_ASSERT(strlen(optarg) > 0,
					"Iter_max is not provided");
			tp->iter_max = strtol(optarg, NULL, 10);
			break;
		case 'l':
			TEST_ASSERT(strlen(optarg) > 0,
					"Num of lcores is not provided");
			tp->num_lcores = strtol(optarg, NULL, 10);
			TEST_ASSERT(tp->num_lcores <= RTE_MAX_LCORE,
					"Num of lcores mustn't be greater than %u",
					RTE_MAX_LCORE);
			break;
		case 'i':
			/* indicate fpga fec config required */
			tp->init_device = true;
			break;
		case 'h':
			print_usage(argv[0]);
			return 0;
		default:
			printf("ERROR: Unknown option: -%c\n", opt);
			return -1;
		}

	if (tp->num_ops == 0) {
		printf(
			"WARNING: Num of operations was not provided or was set 0. Set to default (%u)\n",
			DEFAULT_OPS);
		tp->num_ops = DEFAULT_OPS;
	}
	if (tp->burst_sz == 0) {
		printf(
			"WARNING: Burst size was not provided or was set 0. Set to default (%u)\n",
			DEFAULT_BURST);
		tp->burst_sz = DEFAULT_BURST;
	}
	if (tp->num_lcores == 0) {
		printf(
			"WARNING: Num of lcores was not provided or was set 0. Set to value from RTE config (%u)\n",
			rte_lcore_count());
		tp->num_lcores = rte_lcore_count();
	}

	TEST_ASSERT(tp->burst_sz <= tp->num_ops,
			"Burst size (%u) mustn't be greater than num ops (%u)",
			tp->burst_sz, tp->num_ops);

	tp->num_tests = num_tests;
	return 0;
}
static int
run_all_tests(void)
{
	int ret = TEST_SUCCESS;
	struct test_command *t;

	TAILQ_FOREACH(t, &commands_list, next)
		ret |= (int) t->callback();

	return ret;
}

static int
run_parsed_tests(struct test_params *tp)
{
	int ret = TEST_SUCCESS;
	unsigned int i;

	for (i = 0; i < tp->num_tests; ++i)
		ret |= (int) tp->test_to_run[i]->callback();

	return ret;
}

static void
create_reference_ldpc_dec_op(struct rte_bbdev_dec_op *op)
{
	unsigned int i;
	struct op_data_entries *entry;

	op->ldpc_dec = test_vector.ldpc_dec;
	entry = &test_vector.entries[DATA_INPUT];
	for (i = 0; i < entry->nb_segments; ++i)
		op->ldpc_dec.input.length +=
				entry->segments[i].length;
	if (test_vector.ldpc_dec.op_flags &
			RTE_BBDEV_LDPC_HQ_COMBINE_IN_ENABLE) {
		entry = &test_vector.entries[DATA_HARQ_INPUT];
		for (i = 0; i < entry->nb_segments; ++i)
			op->ldpc_dec.harq_combined_input.length +=
				entry->segments[i].length;
	}
}
/* Read flag value 0/1 from bitmap */
static inline bool
check_bit(uint32_t bitmap, uint32_t bitmask)
{
	return bitmap & bitmask;
}
static void
copy_reference_ldpc_dec_op(struct rte_bbdev_dec_op **ops, unsigned int n,
		unsigned int start_idx,
		struct rte_bbdev_op_data *inputs,
		struct rte_bbdev_op_data *hard_outputs,
		struct rte_bbdev_op_data *soft_outputs,
		struct rte_bbdev_op_data *harq_inputs,
		struct rte_bbdev_op_data *harq_outputs,
		struct rte_bbdev_dec_op *ref_op)
{
	unsigned int i;
	struct rte_bbdev_op_ldpc_dec *ldpc_dec = &ref_op->ldpc_dec;
	struct rte_mbuf *m = inputs->data;

	for (i = 0; i < n; ++i) {
		if (ldpc_dec->code_block_mode == 0) {
			ops[i]->ldpc_dec.tb_params.ea =
					ldpc_dec->tb_params.ea;
			ops[i]->ldpc_dec.tb_params.eb =
					ldpc_dec->tb_params.eb;
			ops[i]->ldpc_dec.tb_params.c =
					ldpc_dec->tb_params.c;
			ops[i]->ldpc_dec.tb_params.cab =
					ldpc_dec->tb_params.cab;
			ops[i]->ldpc_dec.tb_params.r =
					ldpc_dec->tb_params.r;
					printf("code block ea %d eb %d c %d cab %d r %d\n",ldpc_dec->tb_params.ea,ldpc_dec->tb_params.eb,ldpc_dec->tb_params.c, ldpc_dec->tb_params.cab, ldpc_dec->tb_params.r);
		} else {
			ops[i]->ldpc_dec.cb_params.e = ldpc_dec->cb_params.e;
		}

		ops[i]->ldpc_dec.basegraph = ldpc_dec->basegraph;
		ops[i]->ldpc_dec.z_c = ldpc_dec->z_c;
		ops[i]->ldpc_dec.q_m = ldpc_dec->q_m;
		ops[i]->ldpc_dec.n_filler = ldpc_dec->n_filler;
		ops[i]->ldpc_dec.n_cb = ldpc_dec->n_cb;
		ops[i]->ldpc_dec.iter_max = ldpc_dec->iter_max;
		ops[i]->ldpc_dec.rv_index = ldpc_dec->rv_index;
		ops[i]->ldpc_dec.op_flags = ldpc_dec->op_flags;
		ops[i]->ldpc_dec.code_block_mode = ldpc_dec->code_block_mode;
		
		printf("reference bg %d zc %d qm %d nfiller n_filler, n_cb %d iter max %d rv %d\n", ldpc_dec->basegraph, ldpc_dec->z_c, ldpc_dec->q_m,ldpc_dec->n_filler,ldpc_dec->n_cb,ldpc_dec->iter_max,ldpc_dec->rv_index);

if (i<10)
printf("input %x\n",inputs[start_idx+i]);

		if (hard_outputs != NULL)
			ops[i]->ldpc_dec.hard_output =
					hard_outputs[start_idx + i];
		if (inputs != NULL)
			ops[i]->ldpc_dec.input =
					inputs[start_idx + i];
		if (soft_outputs != NULL)
			ops[i]->ldpc_dec.soft_output =
					soft_outputs[start_idx + i];
		if (harq_inputs != NULL)
			ops[i]->ldpc_dec.harq_combined_input =
					harq_inputs[start_idx + i];
		if (harq_outputs != NULL)
			ops[i]->ldpc_dec.harq_combined_output =
					harq_outputs[start_idx + i];
					
//					if (i<10)
//printf("ldpc_dec input %x\n",*ops[i]->ldpc_dec.input->data->buf_addr);




	}
}
int32_t nrLDPC_decod_offload(t_nrLDPC_dec_params* p_decParams, int8_t* p_llr, int8_t* p_out, t_nrLDPC_procBuf* p_procBuf, t_nrLDPC_time_stats* p_profiler)
//int32_t nrLDPC_decod_offload(t_nrLDPC_dec_params* p_decParams, uint8_t C, uint8_t rv, uint32_t F, int8_t* w, int8_t* p_out)
{
    uint32_t numIter = 0;
    struct thread_params *t_params_tp;
    /* Allocate memory for thread parameters structure */
    uint16_t num_lcores=1;
    /*	t_params_tp = rte_zmalloc(NULL, num_lcores * sizeof(struct thread_params),
			RTE_CACHE_LINE_SIZE);
	TEST_ASSERT_NOT_NULL(t_params_tp, "Failed to alloc %zuB for t_params",
			RTE_ALIGN(sizeof(struct thread_params) * num_lcores,
				RTE_CACHE_LINE_SIZE));    
   */ 
    uint16_t enq, deq;
	const uint16_t queue_id = 1; //tp->queue_id;
	const uint16_t burst_sz = 128; //tp->op_params->burst_sz;
	const uint16_t num_ops = 128; //tp->op_params->num_to_process;
	struct rte_bbdev_dec_op *ops_enq[num_ops];
	struct rte_bbdev_dec_op *ops_deq[num_ops];
        struct thread_params *tp=&t_params_tp[0];

    //    struct rte_bbdev_dec_op *ref_op = tp->op_params->ref_dec_op;
	struct test_buffers *bufs = NULL;
	int i, j, ret;
	struct rte_bbdev_info info;
	uint16_t num_to_enq;
   	
//int ret; 
int argc_re=4;
char *argv_re[15];
argv_re[0] = "./build/app/testbbdev";
argv_re[1] = "--"; //./build/app/testbbdev"; 
argv_re[2] = "-v";
argv_re[3] = "../../../targets/ARCH/test-bbdev/test_vectors/ldpc_dec_v8480.data";
printf("argcre %d argvre %s %s %s %s\n", argc_re, argv_re[0], argv_re[1], argv_re[2], argv_re[3],argv_re[4]);

        ret = rte_eal_init(argc_re, argv_re);

argc_re = 3;
argv_re[0] = "--"; //./build/app/testbbdev"; 
argv_re[1] = "-v";
argv_re[2] = "../../../targets/ARCH/test-bbdev/test_vectors/ldpc_dec_v8480.data";

//printf("after ......ret %d argc %d argv %s %s %s %s\n", ret,argc, argv[0], argv[1], argv[2], argv[3],argv[4]);
        /* Parse application arguments (after the EAL ones) */
      //  ret = parse_args(argc_re, argv_re, &test_params);
        /*if (ret < 0) {
                print_usage(argv_re[0]);
                return 1;
        }
*/
snprintf(test_params.test_vector_filename,sizeof(test_params.test_vector_filename),"%s", argv_re[2]); 

test_params.num_ops=128;
test_params.burst_sz=128;
test_params.num_lcores=1;		
test_params.num_tests = 1;
run_all_tests();

 /*       bool extDdr = check_bit(ldpc_cap_flags,
			RTE_BBDEV_LDPC_INTERNAL_HARQ_MEMORY_OUT_ENABLE);
	bool loopback = check_bit(ref_op->ldpc_dec.op_flags,
			RTE_BBDEV_LDPC_INTERNAL_HARQ_MEMORY_LOOPBACK);
	bool hc_out = check_bit(ref_op->ldpc_dec.op_flags,
			RTE_BBDEV_LDPC_HQ_COMBINE_OUT_ENABLE);
	t_params_tp = rte_zmalloc(NULL, num_lcores * sizeof(struct thread_params),
			RTE_CACHE_LINE_SIZE);
	TEST_ASSERT_NOT_NULL(t_params_tp, "Failed to alloc %zuB for t_params",
			RTE_ALIGN(sizeof(struct thread_params) * num_lcores,
				RTE_CACHE_LINE_SIZE));
*/
       // throughput_pmd_lcore_ldpc_dec(&t_params_tp[0]);
     /*   ref_op->ldpc_dec.iter_max = get_iter_max();
	ref_op->ldpc_dec.iter_count = ref_op->ldpc_dec.iter_max;

	if (test_vector.op_type != RTE_BBDEV_OP_NONE)
		copy_reference_ldpc_dec_op(ops_enq, num_ops, 0, bufs->inputs,
				bufs->hard_outputs, bufs->soft_outputs,
				bufs->harq_inputs, bufs->harq_outputs, ref_op);

	for (j = 0; j < num_ops; ++j)
		ops_enq[j]->opaque_data = (void *)(uintptr_t)j;

	for (i = 0; i < TEST_REPETITIONS; ++i) {
		for (j = 0; j < num_ops; ++j) {
			if (!loopback)
				mbuf_reset(
				ops_enq[j]->ldpc_dec.hard_output.data);
			if (hc_out || loopback)
				mbuf_reset(
				ops_enq[j]->ldpc_dec.harq_combined_output.data);
		}
		if (extDdr) {
			bool preload = i == (TEST_REPETITIONS - 1);
			preload_harq_ddr(tp->dev_id, queue_id, ops_enq,
					num_ops, preload);
		}

		for (enq = 0, deq = 0; enq < num_ops;) {
			num_to_enq = burst_sz;

			if (unlikely(num_ops - enq < num_to_enq))
				num_to_enq = num_ops - enq;
				

			enq += rte_bbdev_enqueue_ldpc_dec_ops(tp->dev_id,
					queue_id, &ops_enq[enq], num_to_enq);

			deq += rte_bbdev_dequeue_ldpc_dec_ops(tp->dev_id,
					queue_id, &ops_deq[deq], enq - deq);
		}

		while (deq < enq) {
			deq += rte_bbdev_dequeue_ldpc_dec_ops(tp->dev_id,
					queue_id, &ops_deq[deq], enq - deq);
		}

	}
*/
    return numIter;
}

