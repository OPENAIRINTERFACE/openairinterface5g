/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

   Contact Information
   OpenAirInterface Admin: openair_admin@eurecom.fr
   OpenAirInterface Tech : openair_tech@eurecom.fr
   OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

   Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

#ifndef __THREAD_IPC_H__
#define __THREAD_IPC_H__


#define SUB_FRAME_LENGTH 7680
#define FRAME_LENGTH 76800

#define UE_UL_DELAY 6       /*设置上行组帧在同步时钟基础上的延时（子帧个数）*/
#define UE_UL_SEND_DELAY 6      /*上行发送子帧号在接收子帧时间戳上的延时（子帧个数）*/

#define BUFFERMAX 5         /*环形缓冲区个数*/
#define LOWBUFFER 3                 /*环形缓冲区下限, 不能为0*/
#define HIGHBUFFER 4                /*环形缓冲区上限*/

typedef struct {
  long long subframe_num;     /*子帧编号*/
  int buffer[SUB_FRAME_LENGTH * 2]; /*一子帧数据*/
} buffer_t;

typedef struct {
  int packet_num;         /*环形缓冲区数据包计数器，表示环形缓冲区有效数据包个数*/
  int isfull;           /*标记环形缓冲区有效数据包是否达到上限标志*/
  int isempty;          /*标记环形缓冲区有效数据包是否达到下限标志*/

  pthread_mutex_t buffer_mutex; /*环形缓冲区操作保护锁*/
  pthread_cond_t full_cond;   /*环形缓冲区上限条件变量,配合isfull使用 */
  pthread_cond_t empty_cond;    /*环形缓冲区下限条件变量,配合isempty使用*/

  buffer_t loop_buf[BUFFERMAX]; /*环形缓冲区*/
} loop_buffer_op_t;

typedef struct {
  int decoding_subframe_num;    /*待解码子帧的编号*/
  pthread_mutex_t buffer_mutex; /*对sync_buffer临界区的保护锁*/
  int sync_buffer[SUB_FRAME_LENGTH * 10];  /*同步线程与下行解码线程数据共享缓冲区*/
} sync_buffer_t;

typedef struct {
  unsigned int rx_timestamp;    /*接收数据包第一个sample时间戳*/
  unsigned int tx_timestamp;    /*待发送数据包的时间戳*/
  unsigned long long current_subframe;  /*当前子帧编号*/

  int ue_sync_state;        /*表示UE是否在同步状态，0表示失同步，1表示同步*/
  pthread_mutex_t sync_mutex;   /*根据ue_sync_state标志，唤醒组帧线程*/

  pthread_mutex_t ul_send_mutex;  /*用于sync线程唤醒发送线程发送数据*/
  pthread_mutex_t dl_decode_mutex;/*下行解码保护锁，用于sync线程唤醒解码线程解码*/

  loop_buffer_op_t loop_buffer;
  sync_buffer_t sync_buffer;
} g_thread_ipc_t;

void loop_buffer_reset(buffer_t *loop_buf);
int thread_ipc_init(void);
int thread_ipc_deinit(void);
int set_thread_attr(pthread_attr_t *attr, int policy, int priority, int cpuid);
int find_subframe_num(unsigned long long current_subframe_num, buffer_t *buf, int *flag);
int ue_unsync_thread_ipc_reset(void);

extern g_thread_ipc_t thread_ipc;

#endif
