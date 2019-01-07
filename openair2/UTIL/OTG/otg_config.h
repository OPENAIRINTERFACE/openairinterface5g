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

/*! \file otg_config.h main used structures
* \brief otg structure
* \author  Navid Nikaein and A. Hafsaoui
* \date 2011 - 2014
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning

*/

#ifndef __OTG_CONFIG_H__
# define __OTG_CONFIG_H__


/*!\brief It indicates that: the payload and the transport header (not otg header) are copied from static strings (HEADER_STRING and PAYLOAD_STRING) */
#define STRING_STATIC 1

/*!\brief Define the max number of states*/
#define MAX_NUM_TRAFFIC_STATE 4 //  we have 4 state: OFF, PU, ED, PE

/*!\brief Define the max number of application runing in the  same time*/
#define MAX_NUM_APPLICATION 5

/*!\brief Define the max number of service */
#define NUMBER_OF_SERVICE_MAX 2

/*!\brief Define the max number of traffic runing in the  same time - using their ID in the customized traffic*/
#define MAX_EMU_TRAFFIC 10

#define OTG_FLAG_SIZE 3
#define OTG_FLAG "OTG"

#define IDT_TH 100
#define SIZE_COEF 1000
#define MAXIDT_TIME 1000000 //(1 second = 1000000 microseconds)


/*!\brief Define the max size of the TX buffer */
#define MAX_BUFF_TX 10000

/*!\brief Define the min size of the payload*/
#define PAYLOAD_MIN  1 /*46*/    //http://tools.ietf.org/html/rfc894

/*!\brief Define the min size of the payload, according to the Jumbo frame*/
#ifdef JUMBO_FRAME
#define PAYLOAD_MAX 9000
#else
#define PAYLOAD_MAX 1500 //http://tools.ietf.org/html/rfc894
#endif


/*!\brief Define the size of IP version header, in bytes*/
#define HDR_IP_v4_MIN 20
#define HDR_IP_v4_MAX 60
#define HDR_IP_v6 60

/*!\brief Define the size of TCP header, in bytes*/
#define HDR_TCP 20

/*!\brief Define the size of UDP header, in bytes*/
#define HDR_UDP 8

/*!\brief Define the size of RTP header, in bytes*/
#define RTP_HEADER 12

/*!\brief Define the max size of the header*/
#define MAX_HEADER_SIZE  80


/*!\brief Define the Alphabet string to generate the payload of the packet*/
#define ALPHABET_NUM_LETTER "abcdefghijklmnopqrstuvwyzABCDEFGHIGKLMNOPQRSTUVWXYZ0123456789"

#define ALPHABET_LETTER "abcdefghijklmnopqrstuvwyzABCDEFGHIGKLMNOPQRSTUVWXZ"

/*!\brief Define the Alphabet string to generate the header of the packet*/
#define ALPHABET_NUM "0123456789"

/*!\brief Define the static string to generate the header of the packet*/
//#define HEADER_STRING "5048717272261061594909177115977673656394812939088509638561159848103044447631759621785741859753883189"

/*!\brief Define the static string to generate the payload of the packet*/
//#define PAYLOAD_STRING "UVk5miARQfZGDFKf1wS0dt57kHigd0fXNrUZCjIhpyOS4pZWMHOP1GPdgXmlPtarLUjd3Rmkg05bhUZWtDDmdhrl5EzMZz6DkhIg0Uq7NlaU8ZGrt9EzgVLdr9SiBOLLXiTN3aMInMrlDYFYZ8n5WYbfZTnpz13lbMY4OBE4eWfIMLvBLLyzzzEqjUGILBVMfKGVccPi0VSCyg28RqAiR3z1P6zryk4FWFp0G78AUT1hZWhGcGOTDcKj9bCzny592m1Dj123KWczIm5KVLupO7AP83flqamimfLz6GtHrz5ZN2BAEVQjUhYSc35s5jDhofIlL2U4qPT3Ilsd7amTjaCl5zE0L89ZeIcPCWKSEuNdH5gG8sojuSvph1hU0gG4QOLhCk15IE8eCeMCz2LTL68U0hEQqeM6UmgmA9j7Eid7oPzQHbzj8A30HzGXGhWpt4CT3MSwWVvcCWSbYjkYGgOhHj5csTsONWyGAh5l3qquf8v3jGRSRu0nGXqYILCkw1SX9Na46qodrN6BnPl49djH2AuAaYKAStoR9oL7I1aZG6rVLFPMIZiAqF1tuDVcX9VWnyTVpTMXR6GtBp5bgfDyKuT4ZE9MDUASikGA5hoMfX5Gf2Ml7eLGBtEqZF4rouczHI0DRfgX4ev967n6dYFFkaXbFTvWdykN5bfMinzcrWeqVrmZhTvtUkvq3Rc9enM9qTNz6cDo0HHM0VD8EYtpaPH3yG2CYGDgogHlkaCcHaOyViyq8RH8wf4WQWoHuTNG1kWdkpgTrWic5Gv5p24O9YAPMOn6A1IsdvwpOF85qj8nPvj4nfIo385HOjGfadzfBXueruaKEa0lvbhLgS1bQWKv5fE7k2cMPzQ8USIpUyBhBGUHsLKaykvsr1qDTueAUWAGH8VqyozZZkyhWahjmFEEwU6hhcK1Z9wv9jOAAeqopQvbQFm4aQzzBwGIAhBqhZMiarIBwYPOFdPmK1hKHIa94GGtQbMZ0n83IGt6w8K3dqfOhmpQWqSRZscFwPuo4uhC0ByoC9hFpnizCBfoRZ7Gj9cGOzVLT2eMtD0XC8rUnDiR3p7Ke4ho6lWMLHmtCr7VWYIpY19dtiWoyU0FQ7VMlHoWeBhIUfuG54WVVX0h5Mvvvo0cnLQzh4knysVhAfQw9EhXq94mLrI9GydUaTihOvydQikfq2CrvLeNK81msBlYYoK0aT7OewTUI51YYufj7kYGkPVDf7t5n3VnMV3ShMERKwFyTNHQZyo9ccFibYdoT1FyMAfVeMDO89bUMKAD7RFaT9kUZpaIiH5W7dIbPcPPdSBSr1krKPtuQEeHVgf4OcQLfpOWtsEya4ftXpNw76RPCXmp4rjKt1mCh0pBiTYkQ5GDnj2khLZMzb1uua6R1ika8ACglrs1n0vDbnNjZEVpIMK4OGLFOXIOn9UBserI4Pa63PhUl49TGLNjiqQWdnAsolTKrcjnSklN1swcmyVU8B5gTO4Y3vhkG2U2"

//#define PAYLOAD_STRING "BAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAKF"
/*!\brief We describe below different M2M domains and we present their associated latencies in LTE/LTE-A network REF: "Latency for Real-Time Machine-to-Machine Communication in LTE-Based System Architecture"*/

/*!\brief Latency budget for M2M capillary domain(unit millisecond)*/
#define APPLICATION_PROCESSING_DEVICE_DELAY 1
#define MIN_APPLICATION_PROCESSING_GATEWAY_DELAY 1
#define MAX_APPLICATION_PROCESSING_GATEWAY_DELAY 3
#define MIN_FORMATING_TRANSFERRING_DELAY 1
#define MAX_FORMATING_TRANSFERRING_DELAY 3
#define MIN_ACCESS_DELAY 1.5
#define MAX_ACCESS_DELAY 20
#define TERMINAL_ACCESS_DELAY 1

/*!\brief Latency budget for access domain(unit millisecond)*/
#define MIN_U_PLANE_GW_PROCESSING_DELAY 1
#define MAX_U_PLANE_GW_PROCESSING_DELAY 4
#define MIN_U_PLANE_CORE_IP_ACCESS_DELAY 1
#define MAX_U_PLANE_CORE_IP_ACCESS_DELAY 2
#define MIN_FW_PROXY_DELAY 1
#define MAX_FW_PROXY_DELAY 2

/*!\brief Latency budget for core and application domain(unit millisecond)*/
#define MIN_NETWORK_ACCESS_DELAY 15
#define MAX_NETWORK_ACCESS_DELAY 150
#define MIN_SERVICE_ENABLERS_DELAY 300
#define MAX_SERVICE_ENABLERS_DELAY 500
#define MIN_APPLICATION_ACESS_DELAY 1
#define MAX_APPLICATION_ACESS_DELAY 3

#endif
