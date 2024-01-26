/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2017 Intel Corporation
 */

/*!\file nrLDPC_decoder_offload.c
 * \brief Defines the LDPC decoder
 * \author openairinterface
 * \date 12-06-2021
 * \version 1.0
 * \note: based on testbbdev test_bbdev_perf.c functions. Harq buffer offset added.
 * \mbuf and mempool allocated at the init step, LDPC parameters updated from OAI.
 * \warning
 */

#include <stdint.h>
#include "PHY/sse_intrin.h"
#include "nrLDPCdecoder_defs.h"
#include "nrLDPC_types.h"
#include "nrLDPC_init.h"
#include "nrLDPC_mPass.h"
#include "nrLDPC_cnProc.h"
#include "nrLDPC_bnProc.h"
#include <common/utils/LOG/log.h>
#define NR_LDPC_ENABLE_PARITY_CHECK

#ifdef NR_LDPC_DEBUG_MODE
#include "nrLDPC_tools/nrLDPC_debug.h"
#endif

#include "openair1/PHY/CODING/nrLDPC_extern.h"

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
#include <rte_pdump.h>
#include "nrLDPC_offload.h"

#include <math.h>

#include <rte_dev.h>
#include <rte_launch.h>
#include <rte_bbdev.h>
#include <rte_malloc.h>
#include <rte_random.h>
#include <rte_hexdump.h>
#include <rte_interrupts.h>

// this socket is the NUMA socket, so the hardware CPU id (numa is complex)
#define GET_SOCKET(socket_id) (((socket_id) == SOCKET_ID_ANY) ? 0 : (socket_id))

#define MAX_QUEUES 16

#define OPS_CACHE_SIZE 256U
#define OPS_POOL_SIZE_MIN 511U /* 0.5K per queue */

#define SYNC_WAIT 0
#define SYNC_START 1
#define INVALID_OPAQUE -1
#define TIME_OUT_POLL 1e8
#define INVALID_QUEUE_ID -1
/* Increment for next code block in external HARQ memory */
#define HARQ_INCR 32768
/* Headroom for filler LLRs insertion in HARQ buffer */
#define FILLER_HEADROOM 1024

pthread_mutex_t encode_mutex;
pthread_mutex_t decode_mutex;

const char *typeStr[] = {
    "RTE_BBDEV_OP_NONE", /**< Dummy operation that does nothing */
    "RTE_BBDEV_OP_TURBO_DEC", /**< Turbo decode */
    "RTE_BBDEV_OP_TURBO_ENC", /**< Turbo encode */
    "RTE_BBDEV_OP_LDPC_DEC", /**< LDPC decode */
    "RTE_BBDEV_OP_LDPC_ENC", /**< LDPC encode */
    "RTE_BBDEV_OP_TYPE_COUNT", /**< Count of different op types */
};

/* Represents tested active devices */
struct active_device {
  const char *driver_name;
  uint8_t dev_id;
  int dec_queue;
  int enc_queue;
  uint16_t queue_ids[MAX_QUEUES];
  uint16_t nb_queues;
  struct rte_mempool *bbdev_dec_op_pool;
  struct rte_mempool *bbdev_enc_op_pool;
  struct rte_mempool *in_mbuf_pool;
  struct rte_mempool *hard_out_mbuf_pool;
  struct rte_mempool *soft_out_mbuf_pool;
  struct rte_mempool *harq_in_mbuf_pool;
  struct rte_mempool *harq_out_mbuf_pool;
} active_devs[RTE_BBDEV_MAX_DEVS];
static int nb_active_devs;

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
  struct rte_mempool *mp_dec;
  struct rte_mempool *mp_enc;
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
  struct nrLDPCoffload_params *p_offloadParams;
  uint8_t *p_out;
  uint8_t r;
  uint8_t harq_pid;
  uint8_t ulsch_id;
  rte_atomic16_t nb_dequeued;
  rte_atomic16_t processing_status;
  rte_atomic16_t burst_sz;
  struct test_op_params *op_params;
  struct rte_bbdev_dec_op *dec_ops[MAX_BURST];
  struct rte_bbdev_enc_op *enc_ops[MAX_BURST];
};

// DPDK BBDEV copy
static inline void
mbuf_reset(struct rte_mbuf *m)
{
  m->pkt_len = 0;

  do {
    m->data_len = 0;
    m = m->next;
  } while (m != NULL);
}

/* Read flag value 0/1 from bitmap */
// DPDK BBDEV copy
static inline bool
check_bit(uint32_t bitmap, uint32_t bitmask)
{
  return bitmap & bitmask;
}

/* calculates optimal mempool size not smaller than the val */
// DPDK BBDEV copy
static unsigned int
optimal_mempool_size(unsigned int val)
{
  return rte_align32pow2(val + 1) - 1;
}

// DPDK BBDEV modified - sizes passed to the function, use of data_len and nb_segments, remove code related to Soft outputs, HARQ
// inputs, HARQ outputs
static int create_mempools(struct active_device *ad, int socket_id, uint16_t num_ops, int out_buff_sz, int in_max_sz)
{
  unsigned int ops_pool_size, mbuf_pool_size, data_room_size = 0;
  uint8_t nb_segments = 1;
  ops_pool_size = optimal_mempool_size(RTE_MAX(
      /* Ops used plus 1 reference op */
      RTE_MAX((unsigned int)(ad->nb_queues * num_ops + 1),
              /* Minimal cache size plus 1 reference op */
              (unsigned int)(1.5 * rte_lcore_count() * OPS_CACHE_SIZE + 1)),
      OPS_POOL_SIZE_MIN));

  /* Decoder ops mempool */
  ad->bbdev_dec_op_pool = rte_bbdev_op_pool_create("bbdev_op_pool_dec", RTE_BBDEV_OP_LDPC_DEC,
  /* Encoder ops mempool */                         ops_pool_size, OPS_CACHE_SIZE, socket_id);
  ad->bbdev_enc_op_pool = rte_bbdev_op_pool_create("bbdev_op_pool_enc", RTE_BBDEV_OP_LDPC_ENC,
                                                    ops_pool_size, OPS_CACHE_SIZE, socket_id);

  if ((ad->bbdev_dec_op_pool == NULL) || (ad->bbdev_enc_op_pool == NULL))
    TEST_ASSERT_NOT_NULL(NULL, "ERROR Failed to create %u items ops pool for dev %u on socket %d.",
                         ops_pool_size, ad->dev_id, socket_id);

  /* Inputs */
  mbuf_pool_size = optimal_mempool_size(ops_pool_size * nb_segments);
  data_room_size = RTE_MAX(in_max_sz + RTE_PKTMBUF_HEADROOM + FILLER_HEADROOM, (unsigned int)RTE_MBUF_DEFAULT_BUF_SIZE);
  ad->in_mbuf_pool = rte_pktmbuf_pool_create("in_mbuf_pool", mbuf_pool_size, 0, 0, data_room_size, socket_id);
  TEST_ASSERT_NOT_NULL(ad->in_mbuf_pool,
                       "ERROR Failed to create %u items input pktmbuf pool for dev %u on socket %d.",
                       mbuf_pool_size, ad->dev_id, socket_id);

  /* Hard outputs */
  data_room_size = RTE_MAX(out_buff_sz + RTE_PKTMBUF_HEADROOM + FILLER_HEADROOM, (unsigned int)RTE_MBUF_DEFAULT_BUF_SIZE);
  ad->hard_out_mbuf_pool = rte_pktmbuf_pool_create("hard_out_mbuf_pool", mbuf_pool_size, 0, 0, data_room_size, socket_id);
  TEST_ASSERT_NOT_NULL(ad->hard_out_mbuf_pool,
                       "ERROR Failed to create %u items hard output pktmbuf pool for dev %u on socket %d.",
                       mbuf_pool_size, ad->dev_id, socket_id);
  return 0;
}

const char *ldpcenc_flag_bitmask[] = {
    /** Set for bit-level interleaver bypass on output stream. */
    "RTE_BBDEV_LDPC_INTERLEAVER_BYPASS",
    /** If rate matching is to be performed */
    "RTE_BBDEV_LDPC_RATE_MATCH",
    /** Set for transport block CRC-24A attach */
    "RTE_BBDEV_LDPC_CRC_24A_ATTACH",
    /** Set for code block CRC-24B attach */
    "RTE_BBDEV_LDPC_CRC_24B_ATTACH",
    /** Set for code block CRC-16 attach */
    "RTE_BBDEV_LDPC_CRC_16_ATTACH",
    /** Set if a device supports encoder dequeue interrupts. */
    "RTE_BBDEV_LDPC_ENC_INTERRUPTS",
    /** Set if a device supports scatter-gather functionality. */
    "RTE_BBDEV_LDPC_ENC_SCATTER_GATHER",
    /** Set if a device supports concatenation of non byte aligned output */
    "RTE_BBDEV_LDPC_ENC_CONCATENATION",
};

const char *ldpcdec_flag_bitmask[] = {
    /** Set for transport block CRC-24A checking */
    "RTE_BBDEV_LDPC_CRC_TYPE_24A_CHECK",
    /** Set for code block CRC-24B checking */
    "RTE_BBDEV_LDPC_CRC_TYPE_24B_CHECK",
    /** Set to drop the last CRC bits decoding output */
    "RTE_BBDEV_LDPC_CRC_TYPE_24B_DROP"
    /** Set for bit-level de-interleaver bypass on Rx stream. */
    "RTE_BBDEV_LDPC_DEINTERLEAVER_BYPASS",
    /** Set for HARQ combined input stream enable. */
    "RTE_BBDEV_LDPC_HQ_COMBINE_IN_ENABLE",
    /** Set for HARQ combined output stream enable. */
    "RTE_BBDEV_LDPC_HQ_COMBINE_OUT_ENABLE",
    /** Set for LDPC decoder bypass.
     *  RTE_BBDEV_LDPC_HQ_COMBINE_OUT_ENABLE must be set.
     */
    "RTE_BBDEV_LDPC_DECODE_BYPASS",
    /** Set for soft-output stream enable */
    "RTE_BBDEV_LDPC_SOFT_OUT_ENABLE",
    /** Set for Rate-Matching bypass on soft-out stream. */
    "RTE_BBDEV_LDPC_SOFT_OUT_RM_BYPASS",
    /** Set for bit-level de-interleaver bypass on soft-output stream. */
    "RTE_BBDEV_LDPC_SOFT_OUT_DEINTERLEAVER_BYPASS",
    /** Set for iteration stopping on successful decode condition
     *  i.e. a successful syndrome check.
     */
    "RTE_BBDEV_LDPC_ITERATION_STOP_ENABLE",
    /** Set if a device supports decoder dequeue interrupts. */
    "RTE_BBDEV_LDPC_DEC_INTERRUPTS",
    /** Set if a device supports scatter-gather functionality. */
    "RTE_BBDEV_LDPC_DEC_SCATTER_GATHER",
    /** Set if a device supports input/output HARQ compression. */
    "RTE_BBDEV_LDPC_HARQ_6BIT_COMPRESSION",
    /** Set if a device supports input LLR compression. */
    "RTE_BBDEV_LDPC_LLR_COMPRESSION",
    /** Set if a device supports HARQ input from
     *  device's internal memory.
     */
    "RTE_BBDEV_LDPC_INTERNAL_HARQ_MEMORY_IN_ENABLE",
    /** Set if a device supports HARQ output to
     *  device's internal memory.
     */
    "RTE_BBDEV_LDPC_INTERNAL_HARQ_MEMORY_OUT_ENABLE",
    /** Set if a device supports loop-back access to
     *  HARQ internal memory. Intended for troubleshooting.
     */
    "RTE_BBDEV_LDPC_INTERNAL_HARQ_MEMORY_LOOPBACK",
    /** Set if a device includes LLR filler bits in the circular buffer
     *  for HARQ memory. If not set, it is assumed the filler bits are not
     *  in HARQ memory and handled directly by the LDPC decoder.
     */
    "RTE_BBDEV_LDPC_INTERNAL_HARQ_MEMORY_FILLERS",
};

// DPDK BBDEV modified - in DPDK this function is named add_bbdev_dev, removed code for RTE_BASEBAND_ACC100, IMO we can also remove
// RTE_LIBRTE_PMD_BBDEV_FPGA_LTE_FEC and RTE_LIBRTE_PMD_BBDEV_FPGA_5GNR_FEC - to be checked
static int add_dev(uint8_t dev_id, struct rte_bbdev_info *info)
{
  int ret;
  struct active_device *ad = &active_devs[nb_active_devs];
  unsigned int nb_queues;
  nb_queues = RTE_MIN(rte_lcore_count(), info->drv.max_num_queues);
  nb_queues = RTE_MIN(nb_queues, (unsigned int)MAX_QUEUES);

  /* Display for debug the capabilities of the card */
  for (int i = 0; info->drv.capabilities[i].type != RTE_BBDEV_OP_NONE; i++) {
    printf("device: %d, capability[%d]=%s\n", dev_id, i, typeStr[info->drv.capabilities[i].type]);
    if (info->drv.capabilities[i].type == RTE_BBDEV_OP_LDPC_ENC) {
      const struct rte_bbdev_op_cap_ldpc_enc cap = info->drv.capabilities[i].cap.ldpc_enc;
      printf("    buffers: src = %d, dst = %d\n   capabilites: ", cap.num_buffers_src, cap.num_buffers_dst);
      for (int j = 0; j < sizeof(cap.capability_flags) * 8; j++)
        if (cap.capability_flags & (1ULL << j))
          printf("%s ", ldpcenc_flag_bitmask[j]);
      printf("\n");
    }
    if (info->drv.capabilities[i].type == RTE_BBDEV_OP_LDPC_DEC) {
      const struct rte_bbdev_op_cap_ldpc_dec cap = info->drv.capabilities[i].cap.ldpc_dec;
      printf("    buffers: src = %d, hard out = %d, soft_out %d, llr size %d, llr decimals %d \n   capabilities: ",
             cap.num_buffers_src,
             cap.num_buffers_hard_out,
             cap.num_buffers_soft_out,
             cap.llr_size,
             cap.llr_decimals);
      for (int j = 0; j < sizeof(cap.capability_flags) * 8; j++)
        if (cap.capability_flags & (1ULL << j))
          printf("%s ", ldpcdec_flag_bitmask[j]);
      printf("\n");
    }
  }

  /* setup device */
  ret = rte_bbdev_setup_queues(dev_id, nb_queues, info->socket_id);
  if (ret < 0) {
    printf("rte_bbdev_setup_queues(%u, %u, %d) ret %i\n", dev_id, nb_queues, info->socket_id, ret);
    return TEST_FAILED;
  }

  /* setup device queues */
  struct rte_bbdev_queue_conf qconf = {
      .socket = info->socket_id,
      .queue_size = info->drv.default_queue_conf.queue_size,
  };

  // Search a queue linked to HW capability ldpc decoding
  qconf.op_type = RTE_BBDEV_OP_LDPC_ENC;
  int queue_id;
  for (queue_id = 0; queue_id < nb_queues; ++queue_id) {
    ret = rte_bbdev_queue_configure(dev_id, queue_id, &qconf);
    if (ret == 0) {
      printf("Found LDCP encoding queue (id=%u) at prio%u on dev%u\n", queue_id, qconf.priority, dev_id);
      qconf.priority++;
      //ret = rte_bbdev_queue_configure(ad->dev_id, queue_id, &qconf);
      ad->enc_queue = queue_id;
      ad->queue_ids[queue_id] = queue_id;
      break;
    }
  }
  TEST_ASSERT(queue_id != nb_queues, "ERROR Failed to configure encoding queues on dev %u", dev_id);

  // Search a queue linked to HW capability ldpc encoding
  qconf.op_type = RTE_BBDEV_OP_LDPC_DEC;
  for (queue_id++; queue_id < nb_queues; ++queue_id) {
    ret = rte_bbdev_queue_configure(dev_id, queue_id, &qconf);
    if (ret == 0) {
      printf("Found LDCP decoding queue (id=%u) at prio%u on dev%u\n", queue_id, qconf.priority, dev_id);
      qconf.priority++;
      //ret = rte_bbdev_queue_configure(ad->dev_id, queue_id, &qconf);
      ad->dec_queue = queue_id;
      ad->queue_ids[queue_id] = queue_id;
      break;
    }
  }
  TEST_ASSERT(queue_id != nb_queues, "ERROR Failed to configure encoding queues on dev %u", dev_id);
  ad->nb_queues = 2;
  return TEST_SUCCESS;
}

// DPDK BBDEV modified - nb_segments used, we are not using struct op_data_entries *ref_entries, but struct rte_mbuf *m_head,
// rte_pktmbuf_reset(m_head) added?  if ((op_type == DATA_INPUT) || (op_type == DATA_HARQ_INPUT)) -> no code in else?
static int init_op_data_objs(struct rte_bbdev_op_data *bufs,
                             uint8_t *input,
                             uint32_t data_len,
                             struct rte_mbuf *m_head,
                             struct rte_mempool *mbuf_pool,
                             const uint16_t n,
                             enum op_data_type op_type,
                             uint16_t min_alignment)
{
  int ret;
  unsigned int i, j;
  bool large_input = false;
  uint8_t nb_segments = 1;
  for (i = 0; i < n; ++i) {
    char *data;
    if (data_len > RTE_BBDEV_LDPC_E_MAX_MBUF) {
      /*
       * Special case when DPDK mbuf cannot handle
       * the required input size
       */
      printf("Warning: Larger input size than DPDK mbuf %u\n", data_len);
      large_input = true;
    }
    bufs[i].data = m_head;
    bufs[i].offset = 0;
    bufs[i].length = 0;

    if ((op_type == DATA_INPUT) || (op_type == DATA_HARQ_INPUT)) {
      if ((op_type == DATA_INPUT) && large_input) {
        /* Allocate a fake overused mbuf */
        data = rte_malloc(NULL, data_len, 0);
        TEST_ASSERT_NOT_NULL(data, "rte malloc failed with %u bytes", data_len);
        memcpy(data, input, data_len);
        m_head->buf_addr = data;
        m_head->buf_iova = rte_malloc_virt2iova(data);
        m_head->data_off = 0;
        m_head->data_len = data_len;
      } else {
        // rte_pktmbuf_reset added
        rte_pktmbuf_reset(m_head);
        data = rte_pktmbuf_append(m_head, data_len);

        TEST_ASSERT_NOT_NULL(data, "Couldn't append %u bytes to mbuf from %d data type mbuf pool", data_len, op_type);

        TEST_ASSERT(data == RTE_PTR_ALIGN(data, min_alignment),
                    "Data addr in mbuf (%p) is not aligned to device min alignment (%u)",
                    data,
                    min_alignment);
        rte_memcpy(data, input, data_len);
      }

      bufs[i].length += data_len;

      for (j = 1; j < nb_segments; ++j) {
        struct rte_mbuf *m_tail = rte_pktmbuf_alloc(mbuf_pool);
        TEST_ASSERT_NOT_NULL(m_tail,
                             "Not enough mbufs in %d data type mbuf pool (needed %d, available %u)",
                             op_type,
                             n * nb_segments,
                             mbuf_pool->size);

        data = rte_pktmbuf_append(m_tail, data_len);
        TEST_ASSERT_NOT_NULL(data, "Couldn't append %u bytes to mbuf from %d data type mbuf pool", data_len, op_type);

        TEST_ASSERT(data == RTE_PTR_ALIGN(data, min_alignment),
                    "Data addr in mbuf (%p) is not aligned to device min alignment (%u)",
                    data,
                    min_alignment);
        rte_memcpy(data, input, data_len);
        bufs[i].length += data_len;

        ret = rte_pktmbuf_chain(m_head, m_tail);
        TEST_ASSERT_SUCCESS(ret, "Couldn't chain mbufs from %d data type mbuf pool", op_type);
      }
    } else {
      /* allocate chained-mbuf for output buffer */
      /*for (j = 1; j < nb_segments; ++j) {
  struct rte_mbuf *m_tail =
  rte_pktmbuf_alloc(mbuf_pool);
  TEST_ASSERT_NOT_NULL(m_tail,
  "Not enough mbufs in %d data type mbuf pool (needed %u, available %u)",
  op_type,
  n * nb_segments,
  mbuf_pool->size);

  ret = rte_pktmbuf_chain(m_head, m_tail);
  TEST_ASSERT_SUCCESS(ret,
  "Couldn't chain mbufs from %d data type mbuf pool",
  op_type);
  }*/
    }
  }

  return 0;
}

// DPDK BBEV copy
static int allocate_buffers_on_socket(struct rte_bbdev_op_data **buffers, const int len, const int socket)
{
  int i;

  *buffers = rte_zmalloc_socket(NULL, len, 0, socket);
  if (*buffers == NULL) {
    printf("WARNING: Failed to allocate op_data on socket %d\n", socket);
    /* try to allocate memory on other detected sockets */
    for (i = 0; i < socket; i++) {
      *buffers = rte_zmalloc_socket(NULL, len, 0, i);
      if (*buffers != NULL)
        break;
    }
  }

  return (*buffers == NULL) ? TEST_FAILED : TEST_SUCCESS;
}

// DPDK BBDEV copy
static void
free_buffers(struct active_device *ad, struct test_op_params *op_params)
{
  rte_mempool_free(ad->bbdev_dec_op_pool);
  rte_mempool_free(ad->bbdev_enc_op_pool);
  rte_mempool_free(ad->in_mbuf_pool);
  rte_mempool_free(ad->hard_out_mbuf_pool);
  rte_mempool_free(ad->soft_out_mbuf_pool);
  rte_mempool_free(ad->harq_in_mbuf_pool);
  rte_mempool_free(ad->harq_out_mbuf_pool);

  for (int i = 0; i < rte_lcore_count(); ++i) {
    for (int j = 0; j < RTE_MAX_NUMA_NODES; ++j) {
      rte_free(op_params->q_bufs[j][i].inputs);
      rte_free(op_params->q_bufs[j][i].hard_outputs);
      rte_free(op_params->q_bufs[j][i].soft_outputs);
      rte_free(op_params->q_bufs[j][i].harq_inputs);
      rte_free(op_params->q_bufs[j][i].harq_outputs);
    }
  }
}

// OAI / DPDK BBDEV modified - in DPDK named copy_reference_dec_op, here we are passing t_nrLDPCoffload_params *p_offloadParams to
// the function, check what is value of n, commented code for code block mode
static void
set_ldpc_dec_op(struct rte_bbdev_dec_op **ops, unsigned int n,
		unsigned int start_idx,
		struct test_buffers *bufs,
		struct rte_bbdev_dec_op *ref_op,
		uint8_t r,
		uint8_t harq_pid,
		uint8_t ulsch_id,
		t_nrLDPCoffload_params *p_offloadParams)
{
  unsigned int i;
  for (i = 0; i < n; ++i) {
    ops[i]->ldpc_dec.cb_params.e = p_offloadParams->E;
    ops[i]->ldpc_dec.basegraph = p_offloadParams->BG;
    ops[i]->ldpc_dec.z_c = p_offloadParams->Z;
    ops[i]->ldpc_dec.q_m = p_offloadParams->Qm;
    ops[i]->ldpc_dec.n_filler = p_offloadParams->F;
    ops[i]->ldpc_dec.n_cb = p_offloadParams->n_cb;
    ops[i]->ldpc_dec.iter_max = 20;
    ops[i]->ldpc_dec.rv_index = p_offloadParams->rv;
    ops[i]->ldpc_dec.op_flags = RTE_BBDEV_LDPC_ITERATION_STOP_ENABLE |
                                RTE_BBDEV_LDPC_INTERNAL_HARQ_MEMORY_IN_ENABLE |
                                RTE_BBDEV_LDPC_INTERNAL_HARQ_MEMORY_OUT_ENABLE |
                                RTE_BBDEV_LDPC_HQ_COMBINE_OUT_ENABLE;
    if (p_offloadParams->setCombIn) {
      ops[i]->ldpc_dec.op_flags |= RTE_BBDEV_LDPC_HQ_COMBINE_IN_ENABLE;
    }
    LOG_D(PHY,"ULSCH %02d HARQPID %02d R %02d COMBIN %d RV %d NCB %05d \n", ulsch_id, harq_pid, r, p_offloadParams->setCombIn, p_offloadParams->rv, p_offloadParams->n_cb);
    ops[i]->ldpc_dec.code_block_mode = 1; // ldpc_dec->code_block_mode;
    ops[i]->ldpc_dec.harq_combined_input.offset = ulsch_id * 64 * LDPC_MAX_CB_SIZE + r * LDPC_MAX_CB_SIZE;
    ops[i]->ldpc_dec.harq_combined_output.offset = ulsch_id * 64 * LDPC_MAX_CB_SIZE  + r * LDPC_MAX_CB_SIZE;
    if (bufs->hard_outputs != NULL)
      ops[i]->ldpc_dec.hard_output = bufs->hard_outputs[start_idx + i];
    if (bufs->inputs != NULL)
      ops[i]->ldpc_dec.input = bufs->inputs[start_idx + i];
    if (bufs->soft_outputs != NULL)
      ops[i]->ldpc_dec.soft_output = bufs->soft_outputs[start_idx + i];
    if (bufs->harq_inputs != NULL)
      ops[i]->ldpc_dec.harq_combined_input = bufs->harq_inputs[start_idx + i];
    if (bufs->harq_outputs != NULL)
      ops[i]->ldpc_dec.harq_combined_output = bufs->harq_outputs[start_idx + i];
  }
}

static void set_ldpc_enc_op(struct rte_bbdev_enc_op **ops,
                            unsigned int n,
                            unsigned int start_idx,
                            struct rte_bbdev_op_data *inputs,
                            struct rte_bbdev_op_data *outputs,
                            struct rte_bbdev_enc_op *ref_op,
                            t_nrLDPCoffload_params *p_offloadParams)
{
  // struct rte_bbdev_op_ldpc_enc *ldpc_enc = &ref_op->ldpc_enc;
  for (int i = 0; i < n; ++i) {
    ops[i]->ldpc_enc.cb_params.e = p_offloadParams->E;
    ops[i]->ldpc_enc.basegraph = p_offloadParams->BG;
    ops[i]->ldpc_enc.z_c = p_offloadParams->Z;
    ops[i]->ldpc_enc.q_m = p_offloadParams->Qm;
    ops[i]->ldpc_enc.n_filler = p_offloadParams->F;
    ops[i]->ldpc_enc.n_cb = p_offloadParams->n_cb;
    ops[i]->ldpc_enc.rv_index = p_offloadParams->rv;
    ops[i]->ldpc_enc.op_flags = RTE_BBDEV_LDPC_RATE_MATCH;
    ops[i]->ldpc_enc.code_block_mode = 1;
    ops[i]->ldpc_enc.output = outputs[start_idx + i];
    ops[i]->ldpc_enc.input = inputs[start_idx + i];
  }
}

// DPDK BBDEV modified - in DPDK called validate_dec_op, int8_t* p_out added, remove code related to op_data_entries, turbo_dec
// replaced by ldpc_dec, removed coderelated to soft_output, memcpy(p_out, data+m->data_off, data_len)
static int retrieve_ldpc_dec_op(struct rte_bbdev_dec_op **ops,
                                const uint16_t n,
                                struct rte_bbdev_dec_op *ref_op,
                                const int vector_mask,
                                uint8_t *p_out)
{
  struct rte_bbdev_op_data *hard_output;
  struct rte_mbuf *m;
  unsigned int i;
  char *data;
  for (i = 0; i < n; ++i) {
    hard_output = &ops[i]->ldpc_dec.hard_output;
    m = hard_output->data;
    uint16_t data_len = rte_pktmbuf_data_len(m) - hard_output->offset;
    data = m->buf_addr;
    memcpy(p_out, data + m->data_off, data_len);
  }
  return 0;
}

static int retrieve_ldpc_enc_op(struct rte_bbdev_enc_op **ops, const uint16_t n, struct rte_bbdev_enc_op *ref_op, uint8_t *p_out)
{
  struct rte_bbdev_op_data *output;
  struct rte_mbuf *m;
  unsigned int i;
  char *data;
  for (i = 0; i < n; ++i) {
    output = &ops[i]->ldpc_enc.output;
    m = output->data;
    uint16_t data_len = min((LDPC_MAX_CB_SIZE) / 8, rte_pktmbuf_data_len(m)); // fix me
    data = m->buf_addr;
    for (int byte = 0; byte < data_len; byte++)
      for (int bit = 0; bit < 8; bit++)
        p_out[byte * 8 + bit] = (data[m->data_off + byte] >> (7 - bit)) & 1;
  }
  return 0;
}

// DPDK BBDEV copy
static int init_test_op_params(struct test_op_params *op_params,
                               enum rte_bbdev_op_type op_type,
                               struct rte_mempool *ops_mp,
                               uint16_t burst_sz,
                               uint16_t num_to_process,
                               uint16_t num_lcores)
{
  int ret = 0;
  if (op_type == RTE_BBDEV_OP_LDPC_DEC) {
    ret = rte_bbdev_dec_op_alloc_bulk(ops_mp, &op_params->ref_dec_op, 1);
    op_params->mp_dec = ops_mp;
  } else {
    ret = rte_bbdev_enc_op_alloc_bulk(ops_mp, &op_params->ref_enc_op, 1);
    op_params->mp_enc = ops_mp;
  }

  TEST_ASSERT_SUCCESS(ret, "rte_bbdev_op_alloc_bulk() failed");

  op_params->burst_sz = burst_sz;
  op_params->num_to_process = num_to_process;
  op_params->num_lcores = num_lcores;
  return 0;
}

// DPDK BBDEV modified - in DPDK called throughput_pmd_lcore_ldpc_dec, code related to extDdr removed
static int
pmd_lcore_ldpc_dec(void *arg)
{
  struct thread_params *tp = arg;
  uint16_t enq, deq;
  const uint16_t queue_id = tp->queue_id;
  const uint16_t burst_sz = tp->op_params->burst_sz;
  const uint16_t num_ops = tp->op_params->num_to_process;
  struct rte_bbdev_dec_op *ops_enq[num_ops];
  struct rte_bbdev_dec_op *ops_deq[num_ops];
  struct rte_bbdev_dec_op *ref_op = tp->op_params->ref_dec_op;
  uint8_t r = tp->r;
  uint8_t harq_pid = tp->harq_pid;
  uint8_t ulsch_id = tp->ulsch_id;
  struct test_buffers *bufs = NULL;
  int i, j, ret;
  struct rte_bbdev_info info;
  uint16_t num_to_enq;
  uint8_t *p_out = tp->p_out;
  t_nrLDPCoffload_params *p_offloadParams = tp->p_offloadParams;

  TEST_ASSERT_SUCCESS((burst_sz > MAX_BURST), "BURST_SIZE should be <= %u", MAX_BURST);
  rte_bbdev_info_get(tp->dev_id, &info);

  bufs = &tp->op_params->q_bufs[GET_SOCKET(info.socket_id)][queue_id];
  while (rte_atomic16_read(&tp->op_params->sync) == SYNC_WAIT)
    rte_pause();
  ret = rte_mempool_get_bulk(tp->op_params->mp_dec, (void **)ops_enq, num_ops);
  // looks like a bbdev internal error for the free operation, workaround here
  ops_enq[0]->mempool = tp->op_params->mp_dec;
  // ret = rte_bbdev_dec_op_alloc_bulk(tp->op_params->mp, ops_enq, num_ops);
  TEST_ASSERT_SUCCESS(ret, "Allocation failed for %d ops", num_ops);

  set_ldpc_dec_op(ops_enq,
                  num_ops,
                  0,
                  bufs,
                  ref_op,
                  r,
                  harq_pid,
                  ulsch_id,
                  p_offloadParams);

  /* Set counter to validate the ordering */
  for (j = 0; j < num_ops; ++j)
    ops_enq[j]->opaque_data = (void *)(uintptr_t)j;

  for (enq = 0, deq = 0; enq < num_ops;) {
    num_to_enq = burst_sz;
    if (unlikely(num_ops - enq < num_to_enq))
      num_to_enq = num_ops - enq;

    enq += rte_bbdev_enqueue_ldpc_dec_ops(tp->dev_id, queue_id, &ops_enq[enq], num_to_enq);
    deq += rte_bbdev_dequeue_ldpc_dec_ops(tp->dev_id, queue_id, &ops_deq[deq], enq - deq);
  }

  /* dequeue the remaining */
  int time_out = 0;
  while (deq < enq) {
    deq += rte_bbdev_dequeue_ldpc_dec_ops(tp->dev_id, queue_id, &ops_deq[deq], enq - deq);
    time_out++;
    DevAssert(time_out <= TIME_OUT_POLL);
  }

  // This if statement is not in DPDK
  if (deq == enq) {
    tp->iter_count = 0;
    /* get the max of iter_count for all dequeued ops */
    for (i = 0; i < num_ops; ++i) {
      tp->iter_count = RTE_MAX(ops_enq[i]->ldpc_dec.iter_count, tp->iter_count);
    }
    ret = retrieve_ldpc_dec_op(ops_deq, num_ops, ref_op, tp->op_params->vector_mask, p_out);
    TEST_ASSERT_SUCCESS(ret, "Validation failed!");
  }

  if (num_ops > 0)
    rte_mempool_put_bulk(ops_enq[0]->mempool, (void **)ops_enq, num_ops);

  // Return the worst decoding number of iterations for all segments
  return tp->iter_count;
}

// DPDK BBDEV copy - in DPDK called throughput_pmd_lcore_ldpc_enc
static int pmd_lcore_ldpc_enc(void *arg)
{
  struct thread_params *tp = arg;
  uint16_t enq, deq;
  const uint16_t queue_id = tp->queue_id;
  const uint16_t burst_sz = tp->op_params->burst_sz;
  const uint16_t num_ops = tp->op_params->num_to_process;
  struct rte_bbdev_enc_op *ops_enq[num_ops];
  struct rte_bbdev_enc_op *ops_deq[num_ops];
  struct rte_bbdev_enc_op *ref_op = tp->op_params->ref_enc_op;
  int j, ret;
  uint16_t num_to_enq;
  uint8_t *p_out = tp->p_out;
  t_nrLDPCoffload_params *p_offloadParams = tp->p_offloadParams;

  TEST_ASSERT_SUCCESS((burst_sz > MAX_BURST), "BURST_SIZE should be <= %u", MAX_BURST);

  struct rte_bbdev_info info;
  rte_bbdev_info_get(tp->dev_id, &info);

  TEST_ASSERT_SUCCESS((num_ops > info.drv.queue_size_lim), "NUM_OPS cannot exceed %u for this device", info.drv.queue_size_lim);

  struct test_buffers *bufs = &tp->op_params->q_bufs[GET_SOCKET(info.socket_id)][queue_id];

  while (rte_atomic16_read(&tp->op_params->sync) == SYNC_WAIT)
    rte_pause();

  ret = rte_mempool_get_bulk(tp->op_params->mp_enc, (void **)ops_enq, num_ops);
  // ret = rte_bbdev_enc_op_alloc_bulk(tp->op_params->mp, ops_enq, num_ops);
  TEST_ASSERT_SUCCESS(ret, "Allocation failed for %d ops", num_ops);
  ops_enq[0]->mempool = tp->op_params->mp_enc;

  set_ldpc_enc_op(ops_enq, num_ops, 0, bufs->inputs, bufs->hard_outputs, ref_op, p_offloadParams);

  /* Set counter to validate the ordering */
  for (j = 0; j < num_ops; ++j)
    ops_enq[j]->opaque_data = (void *)(uintptr_t)j;

  for (j = 0; j < num_ops; ++j)
    mbuf_reset(ops_enq[j]->ldpc_enc.output.data);

  for (enq = 0, deq = 0; enq < num_ops;) {
    num_to_enq = burst_sz;
    if (unlikely(num_ops - enq < num_to_enq))
      num_to_enq = num_ops - enq;

    enq += rte_bbdev_enqueue_ldpc_enc_ops(tp->dev_id, queue_id, &ops_enq[enq], num_to_enq);
    deq += rte_bbdev_dequeue_ldpc_enc_ops(tp->dev_id, queue_id, &ops_deq[deq], enq - deq);
  }
  /* dequeue the remaining */
  int time_out = 0;
  while (deq < enq) {
    deq += rte_bbdev_dequeue_ldpc_enc_ops(tp->dev_id, queue_id, &ops_deq[deq], enq - deq);
    time_out++;
    DevAssert(time_out <= TIME_OUT_POLL);
  }

  ret = retrieve_ldpc_enc_op(ops_deq, num_ops, ref_op, p_out);
  TEST_ASSERT_SUCCESS(ret, "Validation failed!");
  // rte_bbdev_enc_op_free_bulk(ops_enq, num_ops);

  if (num_ops > 0)
    rte_mempool_put_bulk(ops_enq[0]->mempool, (void **)ops_enq, num_ops);

  return ret;
}


/*
 * Test function that determines how long an enqueue + dequeue of a burst
 * takes on available lcores.
 */
// OAI / DPDK BBDEV modified - in DPDK called throughput_test, here we pass more parameters to the function (t_nrLDPCoffload_params
// *p_offloadParams, uint8_t r, ...), many commented lines Removed code which specified which function to use based on the op_type,
// now we are using only pmd_lcore_ldpc_dec for RTE_BBDEV_OP_LDPC_DEC op type. Encoder is RTE_BBDEV_OP_LDPC_ENC op type,
// pmd_lcore_ldpc_enc to be implemented.
int start_pmd_dec(struct active_device *ad,
                  struct test_op_params *op_params,
                  t_nrLDPCoffload_params *p_offloadParams,
                  uint8_t r,
                  uint8_t harq_pid,
                  uint8_t ulsch_id,
                  uint8_t *p_out)
{
  int ret;
  unsigned int lcore_id, used_cores = 0;
  // struct rte_bbdev_info info;
  uint16_t num_lcores;
  // rte_bbdev_info_get(ad->dev_id, &info);
  /* Set number of lcores */
  num_lcores = (ad->nb_queues < (op_params->num_lcores)) ? ad->nb_queues : op_params->num_lcores;
  /* Allocate memory for thread parameters structure */
  struct thread_params *t_params = rte_zmalloc(NULL, num_lcores * sizeof(struct thread_params), RTE_CACHE_LINE_SIZE);
  TEST_ASSERT_NOT_NULL(t_params,
                       "Failed to alloc %zuB for t_params",
                       RTE_ALIGN(sizeof(struct thread_params) * num_lcores, RTE_CACHE_LINE_SIZE));
  rte_atomic16_set(&op_params->sync, SYNC_WAIT);

  /* Master core is set at first entry */
  t_params[0].dev_id = ad->dev_id;
  t_params[0].lcore_id = 15;
  t_params[0].op_params = op_params;
  t_params[0].queue_id = ad->dec_queue;
  used_cores++;
  t_params[0].iter_count = 0;
  t_params[0].p_out = p_out;
  t_params[0].p_offloadParams = p_offloadParams;
  t_params[0].r = r;
  t_params[0].harq_pid = harq_pid;
  t_params[0].ulsch_id = ulsch_id;

  // For now, we never enter here, we don't use the DPDK thread pool
  RTE_LCORE_FOREACH_WORKER(lcore_id) {
    if (used_cores >= num_lcores)
      break;
    t_params[used_cores].dev_id = ad->dev_id;
    t_params[used_cores].lcore_id = lcore_id;
    t_params[used_cores].op_params = op_params;
    t_params[used_cores].queue_id = ad->queue_ids[used_cores];
    t_params[used_cores].iter_count = 0;
    t_params[used_cores].p_out = p_out;
    t_params[used_cores].p_offloadParams = p_offloadParams;
    t_params[used_cores].r = r;
    t_params[used_cores].harq_pid = harq_pid;
    t_params[used_cores].ulsch_id = ulsch_id;
    rte_eal_remote_launch(pmd_lcore_ldpc_dec, &t_params[used_cores++], lcore_id);
  }

  rte_atomic16_set(&op_params->sync, SYNC_START);

  ret = pmd_lcore_ldpc_dec(&t_params[0]);

  /* Master core is always used */
  // for (used_cores = 1; used_cores < num_lcores; used_cores++)
  //	ret |= rte_eal_wait_lcore(t_params[used_cores].lcore_id);

  /* Return if test failed */
  if (ret < 0) {
    rte_free(t_params);
    return ret;
  }

  rte_free(t_params);
  return ret;
}

int32_t start_pmd_enc(struct active_device *ad,
                      struct test_op_params *op_params,
                      t_nrLDPCoffload_params *p_offloadParams,
                      uint8_t *p_out)
{
  int ret;
  unsigned int lcore_id, used_cores = 0;

  uint16_t num_lcores;
  num_lcores = (ad->nb_queues < (op_params->num_lcores)) ? ad->nb_queues : op_params->num_lcores;
  /* Allocate memory for thread parameters structure */
  struct thread_params *t_params = rte_zmalloc(NULL, num_lcores * sizeof(struct thread_params), RTE_CACHE_LINE_SIZE);
  TEST_ASSERT_NOT_NULL(t_params,
                       "Failed to alloc %zuB for t_params",
                       RTE_ALIGN(sizeof(struct thread_params) * num_lcores, RTE_CACHE_LINE_SIZE));
  rte_atomic16_set(&op_params->sync, SYNC_WAIT);

  /* Master core is set at first entry */
  t_params[0].dev_id = ad->dev_id;
  t_params[0].lcore_id = 14;
  t_params[0].op_params = op_params;
  // t_params[0].queue_id = ad->queue_ids[used_cores++];
  used_cores++;
  t_params[0].queue_id = ad->enc_queue;
  t_params[0].iter_count = 0;
  t_params[0].p_out = p_out;
  t_params[0].p_offloadParams = p_offloadParams;

  // For now, we never enter here, we don't use the DPDK thread pool
  RTE_LCORE_FOREACH_WORKER(lcore_id) {
    if (used_cores >= num_lcores)
      break;
    t_params[used_cores].dev_id = ad->dev_id;
    t_params[used_cores].lcore_id = lcore_id;
    t_params[used_cores].op_params = op_params;
    t_params[used_cores].queue_id = ad->queue_ids[1];
    t_params[used_cores].iter_count = 0;
    rte_eal_remote_launch(pmd_lcore_ldpc_enc, &t_params[used_cores++], lcore_id);
  }

  rte_atomic16_set(&op_params->sync, SYNC_START);
  ret = pmd_lcore_ldpc_enc(&t_params[0]);
  if (ret) {
    rte_free(t_params);
    return ret;
  }

  rte_free(t_params);
  return ret;
}

struct test_op_params *op_params = NULL;

struct rte_mbuf *m_head[DATA_NUM_TYPES];

// OAI CODE
int32_t LDPCinit()
{
  pthread_mutex_init(&encode_mutex, NULL);
  pthread_mutex_init(&decode_mutex, NULL);
  int ret;
  int dev_id = 0;
  struct rte_bbdev_info info;
  struct active_device *ad = active_devs;
  char *dpdk_dev = "41:00.0"; //PCI address of the card
  char *argv_re[] = {"bbdev", "-a", dpdk_dev, "-l", "14-15", "--file-prefix=b6", "--"};
  // EAL initialization, if already initialized (init in xran lib) try to probe DPDK device
  ret = rte_eal_init(5, argv_re);
  if (ret < 0) {
    printf("EAL initialization failed, probing DPDK device %s\n", dpdk_dev);
    if (rte_dev_probe(dpdk_dev) != 0) {
      LOG_E(PHY, "T2 card %s not found\n", dpdk_dev);
      return (-1);
    }
  }
  // Use only device 0 - first detected device
  rte_bbdev_info_get(0, &info);
  // Set number of queues based on number of initialized cores (-l option) and driver
  // capabilities
  TEST_ASSERT_SUCCESS(add_dev(dev_id, &info), "Failed to setup bbdev");
  TEST_ASSERT_SUCCESS(rte_bbdev_stats_reset(dev_id), "Failed to reset stats of bbdev %u", dev_id);
  TEST_ASSERT_SUCCESS(rte_bbdev_start(dev_id), "Failed to start bbdev %u", dev_id);

  //the previous calls have populated this global variable (beurk)
  // One more global to remove, not thread safe global op_params
  op_params = rte_zmalloc(NULL, sizeof(struct test_op_params), RTE_CACHE_LINE_SIZE);
  TEST_ASSERT_NOT_NULL(op_params, "Failed to alloc %zuB for op_params",
                       RTE_ALIGN(sizeof(struct test_op_params), RTE_CACHE_LINE_SIZE));

  int socket_id = GET_SOCKET(info.socket_id);
  int out_max_sz = 8448; // max code block size (for BG1), 22 * 384
  int in_max_sz = LDPC_MAX_CB_SIZE; // max number of encoded bits (for BG2 and MCS0)
  int num_ops = 1;
  int f_ret = create_mempools(ad, socket_id, num_ops, out_max_sz, in_max_sz);
  if (f_ret != TEST_SUCCESS) {
    printf("Couldn't create mempools");
    return -1;
  }
  // get_num_lcores() hardcoded to 1: we use one core for decode, and another for encode
  // this code from bbdev test example is not considering encode and decode test
  // get_num_ops() replaced by 1: LDPC decode and ldpc encode (7th param)
  f_ret = init_test_op_params(op_params, RTE_BBDEV_OP_LDPC_DEC, ad->bbdev_dec_op_pool, 1, 1, 1);
  f_ret |= init_test_op_params(op_params, RTE_BBDEV_OP_LDPC_ENC, ad->bbdev_enc_op_pool, 1, 1, 1);
  if (f_ret != TEST_SUCCESS) {
    printf("Couldn't init test op params");
    return -1;
  }

  // fill_queue_buffers -> allocate_buffers_on_socket
  for (int i = 0; i < ad->nb_queues; ++i) {
    const uint16_t n = op_params->num_to_process;
    struct rte_mempool *in_mp = ad->in_mbuf_pool;
    struct rte_mempool *hard_out_mp = ad->hard_out_mbuf_pool;
    struct rte_mempool *soft_out_mp = ad->soft_out_mbuf_pool;
    struct rte_mempool *harq_in_mp = ad->harq_in_mbuf_pool;
    struct rte_mempool *harq_out_mp = ad->harq_out_mbuf_pool;
    struct rte_mempool *mbuf_pools[DATA_NUM_TYPES] = {in_mp, soft_out_mp, hard_out_mp, harq_in_mp, harq_out_mp};
    uint8_t queue_id = ad->queue_ids[i];
    struct rte_bbdev_op_data **queue_ops[DATA_NUM_TYPES] = {&op_params->q_bufs[socket_id][queue_id].inputs,
                                                            &op_params->q_bufs[socket_id][queue_id].soft_outputs,
                                                            &op_params->q_bufs[socket_id][queue_id].hard_outputs,
                                                            &op_params->q_bufs[socket_id][queue_id].harq_inputs,
                                                            &op_params->q_bufs[socket_id][queue_id].harq_outputs};

    for (enum op_data_type type = DATA_INPUT; type < 3; type += 2) {
      int ret = allocate_buffers_on_socket(queue_ops[type], n * sizeof(struct rte_bbdev_op_data), socket_id);
      TEST_ASSERT_SUCCESS(ret, "Couldn't allocate memory for rte_bbdev_op_data structs");
      m_head[type] = rte_pktmbuf_alloc(mbuf_pools[type]);
      TEST_ASSERT_NOT_NULL(m_head[type],
                           "Not enough mbufs in %d data type mbuf pool (needed %d, available %u)",
                           type,
                           1,
                           mbuf_pools[type]->size);
    }
  }

  return 0;
}

int32_t LDPCshutdown()
{
  struct active_device *ad = active_devs;
  int dev_id = 0;
  struct rte_bbdev_stats stats;
  free_buffers(ad, op_params);
  rte_free(op_params);
  // Stop and close bbdev
  rte_bbdev_stats_get(dev_id, &stats);
  rte_bbdev_stop(dev_id);
  rte_bbdev_close(dev_id);
  memset(active_devs, 0, sizeof(active_devs));
  nb_active_devs = 0;
  return 0;
}

int32_t LDPCdecoder(struct nrLDPC_dec_params *p_decParams,
                    uint8_t harq_pid,
                    uint8_t ulsch_id,
                    uint8_t C,
                    int8_t *p_llr,
                    int8_t *p_out,
                    t_nrLDPC_time_stats *p_profiler,
                    decode_abort_t *ab)
{
  pthread_mutex_lock(&decode_mutex);
  // hardcoded we use first device

  struct active_device *ad = active_devs;
  t_nrLDPCoffload_params offloadParams = {.E = p_decParams->E,
                                          .n_cb = (p_decParams->BG == 1) ? (66 * p_decParams->Z) : (50 * p_decParams->Z),
                                          .BG = p_decParams->BG,
                                          .Z = p_decParams->Z,
                                          .rv = p_decParams->rv,
                                          .F = p_decParams->F,
                                          .Qm = p_decParams->Qm,
                                          .setCombIn = p_decParams->setCombIn};
  struct rte_bbdev_info info;
  rte_bbdev_info_get(ad->dev_id, &info);
  int socket_id = GET_SOCKET(info.socket_id);
  // fill_queue_buffers -> init_op_data_objs
  struct rte_mempool *in_mp = ad->in_mbuf_pool;
  struct rte_mempool *hard_out_mp = ad->hard_out_mbuf_pool;
  struct rte_mempool *soft_out_mp = ad->soft_out_mbuf_pool;
  struct rte_mempool *harq_in_mp = ad->harq_in_mbuf_pool;
  struct rte_mempool *harq_out_mp = ad->harq_out_mbuf_pool;
  struct rte_mempool *mbuf_pools[DATA_NUM_TYPES] = {in_mp, soft_out_mp, hard_out_mp, harq_in_mp, harq_out_mp};
  uint8_t queue_id = ad->dec_queue;
  struct rte_bbdev_op_data **queue_ops[DATA_NUM_TYPES] = {&op_params->q_bufs[socket_id][queue_id].inputs,
                                                          &op_params->q_bufs[socket_id][queue_id].soft_outputs,
                                                          &op_params->q_bufs[socket_id][queue_id].hard_outputs,
                                                          &op_params->q_bufs[socket_id][queue_id].harq_inputs,
                                                          &op_params->q_bufs[socket_id][queue_id].harq_outputs};
  // this should be modified
  // enum rte_bbdev_op_type op_type = RTE_BBDEV_OP_LDPC_DEC;
  for (enum op_data_type type = DATA_INPUT; type < 3; type += 2) {
    int ret = init_op_data_objs(*queue_ops[type],
                                (uint8_t *)p_llr,
                                p_decParams->E,
                                m_head[type],
                                mbuf_pools[type],
                                1,
                                type,
                                info.drv.min_alignment);
    TEST_ASSERT_SUCCESS(ret, "Couldn't init rte_bbdev_op_data structs");
  }
  int ret = start_pmd_dec(ad, op_params, &offloadParams, C, harq_pid, ulsch_id, (uint8_t *)p_out);
  if (ret < 0) {
    printf("Couldn't start pmd dec\n");
    pthread_mutex_unlock(&decode_mutex);
    return (20); // Fix me: we should propoagate max_iterations properly in the call (impp struct)
  }
  pthread_mutex_unlock(&decode_mutex);
  return ret;
}

int32_t LDPCencoder(unsigned char **input, unsigned char **output, encoder_implemparams_t *impp)
{
  pthread_mutex_lock(&encode_mutex);
  // hardcoded to use the first found board
  struct active_device *ad = active_devs;
  int Zc = impp->Zc;
  int BG = impp->BG;
  t_nrLDPCoffload_params offloadParams = {.E = impp->E,
                                          .n_cb = (BG == 1) ? (66 * Zc) : (50 * Zc),
                                          .BG = BG,
                                          .Z = Zc,
                                          .rv = impp->rv,
                                          .F = impp->F,
                                          .Qm = impp->Qm,
                                          .Kr = (impp->K - impp->F + 7) / 8};
  struct rte_bbdev_info info;
  rte_bbdev_info_get(ad->dev_id, &info);
  int socket_id = GET_SOCKET(info.socket_id);
  // fill_queue_buffers -> init_op_data_objs
  struct rte_mempool *in_mp = ad->in_mbuf_pool;
  struct rte_mempool *hard_out_mp = ad->hard_out_mbuf_pool;
  struct rte_mempool *soft_out_mp = ad->soft_out_mbuf_pool;
  struct rte_mempool *harq_in_mp = ad->harq_in_mbuf_pool;
  struct rte_mempool *harq_out_mp = ad->harq_out_mbuf_pool;
  struct rte_mempool *mbuf_pools[DATA_NUM_TYPES] = {in_mp, soft_out_mp, hard_out_mp, harq_in_mp, harq_out_mp};
  uint8_t queue_id = ad->enc_queue;
  struct rte_bbdev_op_data **queue_ops[DATA_NUM_TYPES] = {&op_params->q_bufs[socket_id][queue_id].inputs,
                                                          &op_params->q_bufs[socket_id][queue_id].soft_outputs,
                                                          &op_params->q_bufs[socket_id][queue_id].hard_outputs,
                                                          &op_params->q_bufs[socket_id][queue_id].harq_inputs,
                                                          &op_params->q_bufs[socket_id][queue_id].harq_outputs};
  for (enum op_data_type type = DATA_INPUT; type < 3; type += 2) {
    int ret = init_op_data_objs(*queue_ops[type],
                                *input,
                                offloadParams.Kr,
                                m_head[type],
                                mbuf_pools[type],
                                1,
                                type,
                                info.drv.min_alignment);
    TEST_ASSERT_SUCCESS(ret, "Couldn't init rte_bbdev_op_data structs");
  }
  int ret=start_pmd_enc(ad, op_params, &offloadParams, *output);
  pthread_mutex_unlock(&encode_mutex);
  return ret;
}
