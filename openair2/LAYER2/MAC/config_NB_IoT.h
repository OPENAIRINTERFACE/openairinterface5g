
/*! \file config_NB_IoT.h
 * \brief configured structures used by scheduler
 * \author  NTUST BMW Lab./
 * \date 2017
 * \email: 
 * \version 1.0
 *
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

//#include "NB_IoT_Message_definitions.h"

#define NUMBER_OF_SIBS_MAX_NB_IoT 6

///MIB
typedef enum operationModeInf{
    iNB_IoTand_SamePCI_r13          = 1,
    iNB_IoTand_DifferentPCI_r13     = 2,
    guardband_r13               = 3,
    standalone_r13              = 4
} operationModeInf_t;

///SIB1_SchedulingInfo_NB_IoT_r13
typedef enum si_Periodicity{
    si_Periodicity_rf64=640,
    si_Periodicity_rf128=1280,
    si_Periodicity_rf256=2560,
    si_Periodicity_rf512=5120,
    si_Periodicity_rf1024=10240,
    si_Periodicity_rf2048=20480,
    si_Periodicity_rf4096=40960
} si_Periodicity_NB_IoT;

typedef enum si_RepetitionPattern{
    si_RepetitionPattern_every2ndRF=0,
    si_RepetitionPattern_every4thRF,
    si_RepetitionPattern_every8thRF,
    si_RepetitionPattern_every16thRF
} si_RepetitionPattern_NB_IoT;

typedef enum sib_MappingInfo{
    sib2_v=0x1,
    sib3_v=0x2,
    sib4_v=0x4,
    sib5_v=0x8,
    sib14_v=0x10,
    sib16_v=0x20
} sib_MappingInfo_NB_IoT;

typedef enum si_TB{
    si_TB_56=2,
    si_TB_120=2,
    si_TB_208=8,
    si_TB_256=8,
    si_TB_328=8,
    si_TB_440=8,
    si_TB_552=8,
    si_TB_680=8
} si_TB_NB_IoT;

///RACH_ConfigCommon configuration

typedef enum ra_ResponseWindowSize{
    ra_ResponseWindowSize_pp2=2,
    ra_ResponseWindowSize_pp3=3,
    ra_ResponseWindowSize_pp4=4,
    ra_ResponseWindowSize_pp5=5,
    ra_ResponseWindowSize_pp6=6,
    ra_ResponseWindowSize_pp7=7,
    ra_ResponseWindowSize_pp8=8,
    ra_ResponseWindowSize_pp10=10
} ra_ResponseWindowSize_NB_IoT;

typedef enum mac_ContentionResolutionTimer{
    mac_ContentionResolutionTimer_pp1=1,
    mac_ContentionResolutionTimer_pp2=2,
    mac_ContentionResolutionTimer_pp3=3,
    mac_ContentionResolutionTimer_pp4=4,
    mac_ContentionResolutionTimer_pp8=8,
    mac_ContentionResolutionTimer_pp16=16,
    mac_ContentionResolutionTimer_pp32=32,
    mac_ContentionResolutionTimer_pp64=64
} mac_ContentionResolutionTimer_NB_IoT;

///NPRACH_ConfigSIB configuration

typedef enum nprach_Periodicity{
    nprach_Periodicity_ms40=40,
    nprach_Periodicity_ms80=80,
    nprach_Periodicity_ms160=160,
    nprach_Periodicity_ms240=240,
	nprach_Periodicity_ms320=320,
    nprach_Periodicity_ms640=640,
    nprach_Periodicity_ms1280=1280,
    nprach_Periodicity_ms2560=2560
} nprach_Periodicity_NB_IoT;

typedef enum nprach_StartTime{
    nprach_StartTime_ms8=8,
    nprach_StartTime_ms16=16,
    nprach_StartTime_ms32=32,
    nprach_StartTime_ms64=64,
    nprach_StartTime_ms128=128,
    nprach_StartTime_ms256=256,
    nprach_StartTime_ms512=512,
    nprach_StartTime_ms1024=1024
} nprach_StartTime_NB_IoT;

typedef enum nprach_SubcarrierOffset{
    nprach_SubcarrierOffset_n0=0,
    nprach_SubcarrierOffset_n12=12,
    nprach_SubcarrierOffset_n24=24,
    nprach_SubcarrierOffset_n36=36,
    nprach_SubcarrierOffset_n2=2,
    nprach_SubcarrierOffset_n18=18,
    nprach_SubcarrierOffset_n34=34
} nprach_SubcarrierOffset_NB_IoT;

typedef enum nprach_NumSubcarriers{
    nprach_NumSubcarriers_n12=12,
    nprach_NumSubcarriers_n24=24,
    nprach_NumSubcarriers_n36=36,
    nprach_NumSubcarriers_n48=48
} nprach_NumSubcarriers_NB_IoT;

typedef enum nprach_SubcarrierMSG3_RangeStart{
    nprach_SubcarrierMSG3_RangeStart_zero=0,
    nprach_SubcarrierMSG3_RangeStart_oneThird=1/3,
    nprach_SubcarrierMSG3_RangeStart_twoThird=2/3,
    nprach_SubcarrierMSG3_RangeStart_one=1
} nprach_SubcarrierMSG3_RangeStart_NB_IoT;

typedef enum maxNumPreambleAttemptCE{
    maxNumPreambleAttemptCE_n3=3,
    maxNumPreambleAttemptCE_n4=4,
    maxNumPreambleAttemptCE_n5=5,
    maxNumPreambleAttemptCE_n6=6,
    maxNumPreambleAttemptCE_n7=7,
    maxNumPreambleAttemptCE_n8=8,
    maxNumPreambleAttemptCE_n10=10
} maxNumPreambleAttemptCE_NB_IoT;

typedef enum numRepetitionsPerPreambleAttempt{
    numRepetitionsPerPreambleAttempt_n1=1,
    numRepetitionsPerPreambleAttempt_n2=2,
    numRepetitionsPerPreambleAttempt_n4=4,
    numRepetitionsPerPreambleAttempt_n8=8,
    numRepetitionsPerPreambleAttempt_n16=16,
    numRepetitionsPerPreambleAttempt_n32=32,
    numRepetitionsPerPreambleAttempt_n64=64,
    numRepetitionsPerPreambleAttempt_n128=128
} numRepetitionsPerPreambleAttempt_NB_IoT;

typedef enum npdcch_NumRepetitions_RA{
    npdcch_NumRepetitions_RA_r1=1,
    npdcch_NumRepetitions_RA_r2=2,
    npdcch_NumRepetitions_RA_r4=4,
    npdcch_NumRepetitions_RA_r8=8,
    npdcch_NumRepetitions_RA_r16=16,
    npdcch_NumRepetitions_RA_r32=32,
    npdcch_NumRepetitions_RA_r64=64,
    npdcch_NumRepetitions_RA_r128=128,
    npdcch_NumRepetitions_RA_r256=256,
    npdcch_NumRepetitions_RA_r512=512,
    npdcch_NumRepetitions_RA_r1024=1024,
    npdcch_NumRepetitions_RA_r2048=2048
} npdcch_NumRepetitions_RA_NB_IoT;

typedef enum npdcch_StartSF_CSS_RA{
    npdcch_StartSF_CSS_RA_v1dot5=3/2,
    npdcch_StartSF_CSS_RA_v2=2,
    npdcch_StartSF_CSS_RA_v4=4,
    npdcch_StartSF_CSS_RA_v8=8,
    npdcch_StartSF_CSS_RA_v16=16,
    npdcch_StartSF_CSS_RA_v32=32,
    npdcch_StartSF_CSS_RA_v48=48,
    npdcch_StartSF_CSS_RA_v64=64
} npdcch_StartSF_CSS_RA_NB_IoT;

typedef enum npdcch_Offset_RA{
    zero=0,
    oneEighth=1/8,
    oneFourth=1/4,
    threeEighth=3/8
} npdcch_Offset_RA_NB_IoT;

typedef enum si_window_length_e{
    ms160=160,
    ms320=320,
    ms480=480,
    ms640=640,
    ms960=960,
    ms1280=1280,
    ms1600=1600
} si_window_length_t;

typedef enum si_periodicity_e{
    rf64=640,
    rf128=1280,
    rf256=2560,
    rf512=5120,
	rf1024=10240,
    rf2048=20480,
    rf4096=40960
} si_periodicity_t;

typedef enum si_repetition_pattern_e{
    every2ndRF=20,
    every4thRF=40,
	every8thRF=80,
    every16thRF=160
} si_repetition_pattern_t;

typedef enum si_tb_e{
    b56=2,
    b120=2,
    b208=8,
    b256=8,
    b328=8,
    b440=8,
    b552=8,
    b680=8
} si_tb_t;


typedef struct sibs_NB_IoT_sched_s{
	si_periodicity_t si_periodicity;
    si_repetition_pattern_t si_repetition_pattern;
    sib_MappingInfo_NB_IoT sib_mapping_info;   //bit vector
    si_tb_t si_tb;

} sibs_NB_IoT_sched_t;


///-------------------------------------------------------MAC--------------------------------------------------------------------///
typedef struct sib1_NB_IoT_sched_s{
    int repetitions;    //  4, 8, 16
    int starting_rf;
} sib1_NB_IoT_sched_t;

typedef struct {

    uint32_t    mac_ra_ResponseWindowSize_NB_IoT;
    uint32_t    mac_ContentionResolutionTimer_NB_IoT;

} mac_RACH_ConfigCommon_NB_IoT;

typedef struct {

    uint32_t    mac_nprach_Periodicity_NB_IoT;
    uint32_t    mac_nprach_StartTime_NB_IoT;
    uint32_t    mac_nprach_SubcarrierOffset_NB_IoT;
    uint32_t    mac_nprach_NumSubcarriers_NB_IoT;
    uint32_t    mac_nprach_SubcarrierMSG3_RangeStart_NB_IoT;
    uint32_t    mac_maxNumPreambleAttemptCE_NB_IoT;
    uint32_t    mac_numRepetitionsPerPreambleAttempt_NB_IoT;
	//	css
    uint32_t    mac_npdcch_NumRepetitions_RA_NB_IoT;		//	rmax
    uint32_t    mac_npdcch_StartSF_CSS_RA_NB_IoT;			//	G
    uint32_t    mac_npdcch_Offset_RA_NB_IoT;				//	alpha offset

} mac_NPRACH_ConfigSIB_NB_IoT;

typedef struct{
    //npdcch-NumRepetitions-r13
    uint32_t R_max;
    //npdcch-StartSF-USS-r13
    double G;
    //npdcch-Offset-USS-r13
    double a_offset;
} npdcch_ConfigDedicated_NB_IoT;

typedef struct rrc_config_NB_IoT_s{

    ///MIB
    uint16_t schedulingInfoSIB1_NB_IoT;

    ///SIB1
    uint32_t cellIdentity_NB_IoT;

	sib1_NB_IoT_sched_t sib1_NB_IoT_sched_config;
	///SIBS
	sibs_NB_IoT_sched_t sibs_NB_IoT_sched[NUMBER_OF_SIBS_MAX_NB_IoT];
	si_window_length_t si_window_length;
	uint32_t si_radio_frame_offset;

    ///SIB2 mac_RACH_ConfigCommon_NB_IoT
    mac_RACH_ConfigCommon_NB_IoT mac_RACH_ConfigCommon[3];

    ///SIB2 mac_NPRACH_ConfigSIB_NB_IoT
    mac_NPRACH_ConfigSIB_NB_IoT mac_NPRACH_ConfigSIB[3];

    ///NPDCCH Dedicated config
    npdcch_ConfigDedicated_NB_IoT npdcch_ConfigDedicated[3];

} rrc_config_NB_IoT_t;

#endif
