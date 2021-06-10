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

#define MAX_QUEUES RTE_MAX_LCORE
#define TEST_REPETITIONS 1000
/* Switch between PMD and Interrupt for throughput TC */
static bool intr_enabled;

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
/*static struct test_params {
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
*/
int32_t nrLDPC_decod_offload(t_nrLDPC_dec_params* p_decParams, uint8_t C, uint8_t rv, uint32_t F, int8_t* w, int8_t* p_out)
{
    uint32_t numIter = 0;
    struct thread_params *t_params_tp;
    uint16_t num_lcores=1;
    /*uint16_t enq, deq;
	const uint16_t queue_id = 1; //tp->queue_id;
	const uint16_t burst_sz = 128; //tp->op_params->burst_sz;
	const uint16_t num_ops = 128; //tp->op_params->num_to_process;
	struct rte_bbdev_dec_op *ops_enq[num_ops];
	struct rte_bbdev_dec_op *ops_deq[num_ops];

	struct test_buffers *bufs = NULL;
	int i, j, ret;
	struct rte_bbdev_info info;
	uint16_t num_to_enq;
   */	
        bool extDdr = check_bit(ldpc_cap_flags,
			RTE_BBDEV_LDPC_INTERNAL_HARQ_MEMORY_OUT_ENABLE);

        /* Allocate memory for thread parameters structure */
	t_params_tp = rte_zmalloc(NULL, num_lcores * sizeof(struct thread_params),
			RTE_CACHE_LINE_SIZE);
	TEST_ASSERT_NOT_NULL(t_params_tp, "Failed to alloc %zuB for t_params",
			RTE_ALIGN(sizeof(struct thread_params) * num_lcores,
				RTE_CACHE_LINE_SIZE));

       // throughput_pmd_lcore_ldpc_dec(&t_params_tp[0]);


    return numIter;
}

