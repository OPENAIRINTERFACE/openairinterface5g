<?php
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

/*! \file recep.php
* \brief Process the POST of the OSD form.
* \author Philippe Foubert & Navid Nikaein & Andre Gomes (One Source)
* \date 2014
* \version 0.1
* \company Eurecom, 
* \email: openair_tech@eurecom.fr
* \note
* \warning
*/
?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
  "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en" dir="ltr"> 
<head>
<?
include('header.php'); 
?>                   
</head>
<?
// INCLUDE ALL THE VARIABLES RELATED TO THE PATHs
include('/opt/lampp/htdocs/OSD/OpenAirEmu.conf');
//$GET_OPENAIR_TARGETS=shell_exec("source /opt/lampp/htdocs/OSD/OpenAirEmu.conf");
//echo $GET_OPENAIR_TARGETS;

// TO BE SURE ABOUT THE PATHs
 //echo $OPENAIR_TARGETS.'<br />'.$WEBXML_PATH.'<br />'.$RESULTS_PATH.'<br />'.$OPENAIR1_DIR.'<br />'.$OPENAIR2_DIR.'<br />'.$OPENAIR3_DIR.'<br />';

// TO HIDE ALL THE PHP ERRORS AND WARNINGS
ini_set('display_errors','Off');
ini_set('log_errors',1);
ini_set('error_log','/var/logs/php/errlog.txt');

// *****************************************************************************************
//  COPY and PASTE
//FROM HERE COPY AND PASTE THE CONTENT OF THE FORM RECEPTION OF THE WEBPORTAL VERSION FROM AFTER THIS LINE 
// 
// *****************************************************************************************

function nospace($input) {
	$espace = array(" ");
	return str_replace($espace, "", $input);	
}

// PHIL - To put a 0 or 1 between tags For all the CHECKBOXES so they are not empties- 2012 06 28 
if ( isset($_POST['throughput'])   && $_POST['throughput'] == '1' ) { $THROUGHPUT = 1; } else { $THROUGHPUT = 0; }	
if ( isset($_POST['latency'])   && $_POST['latency'] == '1' ) { $LATENCY = 1; } else { $LATENCY = 0; }
if ( isset($_POST['lossrate'])   && $_POST['lossrate'] == '1' ) { $LOSS_RATE = 1; } else { $LOSS_RATE = 0; }
if ( isset($_POST['pkttrace'])   && $_POST['pkttrace'] == '1' ) { $PKTTRACE = 1; } else { $PKTTRACE = 0; }
if ( isset($_POST['bg_traffic']) &&  $_POST['bg_traffic'] == '1' ) { $BG_TRAFFIC = 1; } else { $BG_TRAFFIC = 0; }
if ( isset($_POST['curves'])     && $_POST['curves'] == '1' ) { $CURVES = enabled; } else { $CURVES = disabled; }
// More parameters
if ( isset($_POST['phy_abstraction']) &&  $_POST['phy_abstraction'] == '1' ) { $PHY_ABSTRACTION = '-a'; } else { $PHY_ABSTRACTION = ''; }
if ( isset($_POST['pcap_trace']) &&  $_POST['pcap_trace'] == '1' ) { $PCAP_TRACE = 1; } else { $PCAP_TRACE = 0; }
if ( isset($_POST['profiling']) && $_POST['profiling'] == '1' ) { $PROFILING = 1; } else { $PROFILING = 0; }
if ( isset($_POST['dl_mcs'])) { $DL_MCS = '-m' . $_POST['dl_mcs']; } else { $DL_MCS = ''; }
if ( isset($_POST['ul_mcs'])) { $UL_MCS = '-t' . $_POST['ul_mcs']; } else { $UL_MCS = ''; }
if ( isset($_POST['snr'])) { $SNR = '-s' . $_POST['snr']; } else { $SNR = ''; }
if ( isset($_POST['transmission_mode'])) { $TRANSMISSION_MODE = $_POST['transmission_mode']; } else { $TRANSMISSION_MODE = ''; }
if ( isset($_POST['frame_type'])) { $FRAME_TYPE = $_POST['frame_type']; } else { $FRAME_TYPE = ''; }
if ( isset($_POST['tdd_config'])) { $TDD_CONFIG = '<TDD_CONFIG>'.$_POST['tdd_config'].'</TDD_CONFIG>'; } else { $TDD_CONFIG = ''; }

// VARIABLES FOR BOTH WEBPORTAL AND PORTABLE VERSION
$espace= array(" ");
$date = date("YmdHis"); 
$PERSON = str_replace($espace, "", $user);
$ID = $PERSON.$date;
$user = $_POST['username'];
$XML_FILENAME = str_replace($espace, "", $user).'.'.$date.'.xml';

// eNB Distribution
if (isset($_POST['eNB_initial_distribution']) && $_POST['eNB_initial_distribution'] == 'fixed') {
$eNB_INITIAL_DISTRIBUTION       =
        '<eNB_INITIAL_DISTRIBUTION>fixed</eNB_INITIAL_DISTRIBUTION>
                <eNB_INITIAL_COORDINATES>
                        <POS_X>'.nospace($_POST['pos_x']).'</POS_X>
                        <POS_Y>'.nospace($_POST['pos_y']).'</POS_Y>
                </eNB_INITIAL_COORDINATES>';
                }
elseif (isset($_POST['eNB_initial_distribution']) && $_POST['eNB_initial_distribution'] == 'random') {
$eNB_INITIAL_DISTRIBUTION       =
        '<eNB_INITIAL_DISTRIBUTION>random</eNB_INITIAL_DISTRIBUTION>
                <RANDOM_eNB_DISTRIBUTION>
                        <NUMBER_OF_CELLS>'.nospace($_POST['random_eNB_distr']).'</NUMBER_OF_CELLS>
                </RANDOM_eNB_DISTRIBUTION>';
                }

// RBs
if (isset($_POST['num_rbs_dl'])) {
	$NUMBER_OF_RBS = '<NUMBER_OF_RBS_DL>'.$_POST['num_rbs_dl'].'</NUMBER_OF_RBS_DL>';
}
else {
	$NUM_RBS = '';
}

// PCAP
if ($PCAP_TRACE == 1) {
	$TRACE_FILE = '<TRACE_FILE>pcap</TRACE_FILE>';
}
else {
	$TRACE_FILE = '';
}

// CLI
if ( isset($_POST['cli_enabled']) && $_POST['cli_enabled'] == '1' ) {
	if ( isset($_POST['cli_start_enb']) && $_POST['cli_start_enb'] == '1' ) { $CLI_START_ENB = 1; } else { $CLI_START_ENB = 0; }
	if ( isset($_POST['cli_start_ue']) && $_POST['cli_start_ue'] == '1' ) { $CLI_START_UE = 1; } else { $CLI_START_UE = 0; }
	$CLI =
	'<CLI>
		<START_ENB>'.$CLI_START_ENB.'</START_ENB>
		<START_UE>'.$CLI_START_UE.'</START_UE>
	</CLI>';
}
else {
	$CLI = '';
}

// ICI TOUTES LES OPTIONS - ELLES SONT COMMENTEES EN BAS DU FICHIER

// <!-- Here is the XML template which will be filled in with the user values -->
$data = '
 
<OAI_EMULATION>
	<ENVIRONMENT_SYSTEM_CONFIG>
		<FADING>
			<LARGE_SCALE>0</LARGE_SCALE>
			<FREE_SPACE_MODEL_PARAMETERS>
				<PATHLOSS_EXPONENT>'.nospace($_POST['pathlossexp']).'</PATHLOSS_EXPONENT> 
				<PATHLOSS_0_dB>'.nospace($_POST['pathlosszero']).'</PATHLOSS_0_dB>
			</FREE_SPACE_MODEL_PARAMETERS>
			<SMALL_SCALE>'.nospace($_POST['smallscale']).'</SMALL_SCALE>
		</FADING>
		<WALL_PENETRATION_LOSS_dB>'.nospace($_POST['wallpenetloss']).'</WALL_PENETRATION_LOSS_dB>
		<SYSTEM_BANDWIDTH_MB>'.nospace($_POST['sysbw']).'</SYSTEM_BANDWIDTH_MB>
		<SYSTEM_FREQUENCY_GHz>'.nospace($_POST['uefreq']).'</SYSTEM_FREQUENCY_GHz>
		'.$NUMBER_OF_RBS.'
		<TRANSMISSION_MODE>'.$TRANSMISSION_MODE.'</TRANSMISSION_MODE>
		<FRAME_CONFIG>
			<FRAME_TYPE>'.$FRAME_TYPE.'</FRAME_TYPE>
			'.$TDD_CONFIG.'
		</FRAME_CONFIG>
		<ANTENNA>
			<eNB_ANTENNA>
				<RX_NOISE_LEVEL_dB>'.nospace($_POST['eNB_RX_noise_power']).'</RX_NOISE_LEVEL_dB>
				<NUMBER_OF_SECTORS>'.nospace($_POST['eNB_nb_sectors']).'</NUMBER_OF_SECTORS>
				<BEAM_WIDTH_dB>'.nospace($_POST['eNB_beam_width']).'</BEAM_WIDTH_dB>
				<ANTENNA_GAIN_dBi>'.nospace($_POST['eNB_antenna_gain']).'</ANTENNA_GAIN_dBi>
				<TX_POWER_dBm>'.nospace($_POST['eNB_TX_power']).'</TX_POWER_dBm>
			</eNB_ANTENNA>
			<UE_ANTENNA>
				<RX_NOISE_LEVEL_dB>'.nospace($_POST['UE_RX_noise_power']).'</RX_NOISE_LEVEL_dB>
				<ANTENNA_GAIN_dBi>'.nospace($_POST['UE_antenna_gain']).'</ANTENNA_GAIN_dBi>
				<TX_POWER_dBm>'.nospace($_POST['UE_TX_power']).'</TX_POWER_dBm>
			</UE_ANTENNA>
		</ANTENNA>
	</ENVIRONMENT_SYSTEM_CONFIG>
	<TOPOLOGY_CONFIG>	
		<AREA>			
		  <X_m>'.nospace($_POST['areax']).'</X_m>
		  <Y_m>'.nospace($_POST['areay']).'</Y_m>	
		</AREA>
		<NETWORK_TYPE>'.nospace($_POST['typetopo']).'</NETWORK_TYPE>
		<CELL_TYPE>'.nospace($_POST['celltype']).'</CELL_TYPE> 
		<RELAY>0</RELAY>
		<MOBILITY> 
		    <UE_MOBILITY>
			<UE_INITIAL_DISTRIBUTION>'.nospace($_POST['uedistr']).'</UE_INITIAL_DISTRIBUTION> 
			<RANDOM_UE_DISTRIBUTION>
				<NUMBER_OF_NODES>'.nospace($_POST['random_UE_dist_nb_node']).'</NUMBER_OF_NODES>
			</RANDOM_UE_DISTRIBUTION>
			<UE_MOBILITY_TYPE>'.nospace($_POST['uemobility']).'</UE_MOBILITY_TYPE> 
				<UE_MOVING_DYNAMICS>
					<MIN_SPEED_mps>'.nospace($_POST['minspeed']).'</MIN_SPEED_mps>
					<MAX_SPEED_mps>'.nospace($_POST['maxspeed']).'</MAX_SPEED_mps>
					<MIN_SLEEP_ms>'.nospace($_POST['minpausetime']).'</MIN_SLEEP_ms>
					<MAX_SLEEP_ms>'.nospace($_POST['maxpausetime']).'</MAX_SLEEP_ms>
					<MIN_JOURNEY_TIME_ms>'.nospace($_POST['minjourneytime']).'</MIN_JOURNEY_TIME_ms>
					<MAX_JOURNEY_TIME_ms>'.nospace($_POST['maxjourneytime']).'</MAX_JOURNEY_TIME_ms>
				</UE_MOVING_DYNAMICS>
			</UE_MOBILITY>
			<eNB_MOBILITY>
   				'.nospace($eNB_INITIAL_DISTRIBUTION).'
				<eNB_MOBILITY_TYPE>'.nospace($_POST['enbmobility']).'</eNB_MOBILITY_TYPE> 
			</eNB_MOBILITY>
		</MOBILITY>
		<OMV>'.nospace($_POST['omv']).'</OMV>
	</TOPOLOGY_CONFIG>
	<APPLICATION_CONFIG>
		<PREDEFINED_TRAFFIC>
			<SOURCE_ID>'.nospace($_POST['pre_sourceid']).'</SOURCE_ID>					
			<APPLICATION_TYPE>'.nospace($_POST['pre_apptype']).'</APPLICATION_TYPE> 
			<DESTINATION_ID>'.nospace($_POST['pre_destid']).'</DESTINATION_ID>
			<BACKGROUND_TRAFFIC>'.$BG_TRAFFIC.'</BACKGROUND_TRAFFIC>
		</PREDEFINED_TRAFFIC>
		<CUSTOMIZED_TRAFFIC>
			<SOURCE_ID>'.nospace($_POST['cust_sourceid']).'</SOURCE_ID> 
			<TRANSPORT_PROTOCOL>'.nospace($_POST['cust_transprot']).'</TRANSPORT_PROTOCOL> 
			<IP_VERSION>'.nospace($_POST['cust_ipversion']).'</IP_VERSION> 
			<DESTINATION_ID>'.nospace($_POST['cust_destid']).'</DESTINATION_ID>
			<IDT_DIST>'.nospace($_POST['idt_dist']).'</IDT_DIST> 
			<IDT_MIN_ms>'.nospace($_POST['idtmin']).'</IDT_MIN_ms>
			<IDT_MAX_ms>'.nospace($_POST['idtmax']).'</IDT_MAX_ms>
			<IDT_STANDARD_DEVIATION>'.nospace($_POST['idtstddeviation']).'</IDT_STANDARD_DEVIATION>
			<IDT_LAMBDA>'.nospace($_POST['idtlambda']).'</IDT_LAMBDA>
			<IDT_SCALE>'.nospace($_POST['idtscale']).'</IDT_SCALE>
			<IDT_SHAPE>'.nospace($_POST['idtshape']).'</IDT_SHAPE>
			<SIZE_DIST>'.nospace($_POST['size_dist']).'</SIZE_DIST>
			<SIZE_MIN_byte>'.nospace($_POST['sizemin']).'</SIZE_MIN_byte>
			<SIZE_MAX_byte>'.nospace($_POST['sizemax']).'</SIZE_MAX_byte>
			<SIZE_STANDARD_DEVIATION>'.nospace($_POST['sizestddeviation']).'</SIZE_STANDARD_DEVIATION>
			<SIZE_LAMBDA>'.nospace($_POST['sizelambda']).'</SIZE_LAMBDA>
			<SIZE_SCALE>'.nospace($_POST['sizescale']).'</SIZE_SCALE>
			<SIZE_SHAPE>'.nospace($_POST['sizeshape']).'</SIZE_SHAPE>
			<DESTINATION_PORT>'.nospace($_POST['destport']).'</DESTINATION_PORT>
			<PU>
			    <PROB_OFF_PU>'.nospace($_POST['proboffpu']).'</PROB_OFF_PU> 
			    <PROB_PU_ED>'.nospace($_POST['probpued']).'</PROB_PU_ED> 
			    <HOLDING_TIME_OFF_PU>'.nospace($_POST['holdtimeoffpu']).'</HOLDING_TIME_OFF_PU> 
			</PU>
			<ED> 
			    <PROB_OFF_ED>'.nospace($_POST['proboffed']).'</PROB_OFF_ED> 
			    <PROB_ED_PE>'.nospace($_POST['probedpe']).'</PROB_ED_PE> 
			    <HOLDING_TIME_OFF_ED>'.nospace($_POST['holdtimeoffed']).'</HOLDING_TIME_OFF_ED> 
			</ED>
			<PE>
			    <HOLDING_TIME_OFF_PE>'.nospace($_POST['holdtimeoffpe']).'</HOLDING_TIME_OFF_PE> 
			</PE>
		</CUSTOMIZED_TRAFFIC>
	</APPLICATION_CONFIG>
	
	<EMULATION_CONFIG>
		<EMULATION_TIME_ms>'.nospace($_POST['emutime']).'</EMULATION_TIME_ms>
		<CURVE>'.$CURVES.'</CURVE>
		<PROFILING>'.$PROFILING.'</PROFILING>
		'.$TRACE_FILE.'
		<PERFORMANCE_METRICS>
			    <THROUGHPUT>'.$THROUGHPUT.'</THROUGHPUT>
			    <LATENCY>'.$LATENCY.'</LATENCY>
			    <LOSS_RATE>'.$LOSS_RATE.'</LOSS_RATE>
			    <OWD_RADIO_ACESS>1</OWD_RADIO_ACESS>
		</PERFORMANCE_METRICS>
		         <LOG> 
			    <LEVEL>'.nospace($_POST['level']).'</LEVEL>
			    <VERBOSITY>'.nospace($_POST['verbosity']).'</VERBOSITY>
			    <INTERVAL>'.nospace($_POST['interval']).'</INTERVAL>
			</LOG>
			<PACKET_TRACE>'.$PKTTRACE.'</PACKET_TRACE>
			<SEED_VALUE>'.nospace($_POST['seedvalue']).'</SEED_VALUE>
			'.$CLI.'
	</EMULATION_CONFIG>
	<PROFILE>'.$XML_FILENAME.'</PROFILE>	
</OAI_EMULATION>
';

//echo $data."<br /><br />";


/*
// VARIABLES FOR WEBPORTAL
$EMU_RESULTS = "/drupal-6.19/CLUSTER/r4g7/emu_results";
$EMU_RESULTS_USER = $EMU_RESULTS.'/'.str_replace($espace, "", $user).'/'.$date;
$FOR_R4G1 = "/opt/lampp/htdocs/drupal-6.19/CLUSTER/r4g7/emu_results/".str_replace($espace, "", $user)."/".$date."/LOGS/";
$CHEMIN = "/nfs/emu_results/".str_replace($espace, "", $user)."/".$date."/LOGS/";
$file = '/opt/lampp/htdocs/drupal-6.19/CLUSTER/r4g7/webxml/'.$XML_FILE;
*/

// VARIABLES FOR PORTABLE VERSION
$xmlfile = $WEBXML_PATH.'/'.str_replace($espace, "", $user).'.'.$date.'.xml';
//echo $xmlfile.'<br />'.$XML_FILENAME.'<br />';
// 1 - WRITTING THE XML FILE 
File_put_contents($xmlfile, $data);
sleep(1);

// IN PORTABLE VERSION THERE IS NO FOR R4G1, SINCE WE WORK ON ONLY ONE MACHINE
// RESULTS_PATH is defined in /opt/lampp/htdocs/OSD/OpenAirEmu.conf file
$RESULTS_PATH_EMULATION = $RESULTS_PATH."/".str_replace($espace, "", $user)."/".$date."/";
//echo $RESULTS_PATH_EMULATION.'<br />';
//$RESULTS_PATH_EMULATION_LOGS = $RESULTS_PATH_EMULATION."LOGS/";
//$RESULTS_PATH_EMULATION_XML = $RESULTS_PATH_EMULATION."SCENARIO/XML/";

// PROFILING and PCAP Copy
if ($PROFILING == 1) {
	$PROFILING_PARSE = '2>profiling.log';
	$PROFILING_COPY = ' ; cp profiling.log '.$RESULTS_PATH_EMULATION;
}
else {
	$PROFILING_PARSE = '';
	$PROFILING_COPY = '';
}
if ($PCAP_TRACE == 1) {
	$PCAP_COPY = ' ; cp oai_opt.pcap '.$RESULTS_PATH_EMULATION;
}
else {
	$PCAP_COPY = '';
}

// -1 : portable, 0: web portal, # for templates  
// 2 - STARTING OAISIM 
// $OAISIM=shell_exec("set -x ; mkdir /tmp/$ID ; cd /tmp/$ID ; export OPENAIR1_DIR=$OPENAIR1_DIR ; export OPENAIR2_DIR=$OPENAIR2_DIR ; export OPENAIR3_DIR=$OPENAIR3_DIR ; export OPENAIR_TARGETS=$OPENAIR_TARGETS ; $OPENAIR_TARGETS/SIMU/USER/oaisim -a -c $XML_FILENAME >oaisim.log ; mv * $RESULTS_PATH_EMULATION_LOGS ; rmdir /tmp/$ID ");

$OAISIM="set -x ; mkdir /tmp/$ID ; cd /tmp/$ID ; export OPENAIR1_DIR=$OPENAIR1_DIR ; export OPENAIR2_DIR=$OPENAIR2_DIR ; export OPENAIR3_DIR=$OPENAIR3_DIR ; export OPENAIR_TARGETS=$OPENAIR_TARGETS ; $OPENAIR_TARGETS/SIMU/USER/oaisim $PHY_ABSTRACTION $SNR $DL_MCS $UL_MCS -c -1 >oaisim.log ".$PROFILING_PARSE." ; cp oaisim.log $RESULTS_PATH_EMULATION".$PROFILING_COPY.$PCAP_COPY;

//echo $OAISIM; 

shell_exec($OAISIM);

// Get hostname
if (version_compare(PHP_VERSION, '5.3.0') >= 0) {
	$hostname = gethostname();
}
elseif (version_compare(PHP_VERSION, '4.2.0') >= 0) {
	$hostname = php_uname('n');
}
elseif (version_compare(PHP_VERSION, '4.2.0') < 0) {
	$hostname = getenv('HOSTNAME'); 
	if(!$hostname) $hostname = trim(`hostname`); 
	if(!$hostname) $hostname = exec('echo $HOSTNAME');
	if(!$hostname) $hostname = preg_replace('#^\w+\s+(\w+).*$#', '$1', exec('uname -a'));
}

// APACHE VARIABLES
$RESULTS_PATH="http://".$hostname."/results/";
$XMLFILE_PATH="http://".$hostname."/xmlfile/";

// PROFILING and PCAP Results
if ($PROFILING == 1) {
	$PROFILING_OUTPUT = '
	<div align="center" class="notice span-9">
		<br>
		<p>Here are the performance profiling statistics :
		<br>
		<a href="'.$RESULTS_PATH.str_replace($espace, "", $user).'/'.$date.'/profiling.log" target="_blank">profiling.log</a></p>
	</div>';
}
else {
	$PROFILING_OUTPUT = '';
}
if ($PCAP_TRACE == 1) {
	$PCAP_OUTPUT = '
	<div align="center" class="notice span-9">
		<br>
		<p>Here is the PCAP File :
		<br>
		<a href="'.$RESULTS_PATH.str_replace($espace, "", $user).'/'.$date.'/oai_opt.pcap" target="_blank">oai_opt.pcap</a></p>
	</div>';
}
else {
	$PCAP_OUTPUT = '';
}

//
// 3 - DISPLAY RESULTS
//

// THE FIRST SQUARE  - XML FILE
echo '
<div class=" span-50">
      <br>
    </div>

    <div class="span-31">
	<div class=" span-10">
	<br>
    </div>

    <div align="center" class=" span-10">
		<div class="success span-9">
			<br>
			<p>Here is the XML file of your OpenAirEmulation :
			<br>
			<a href="'.$RESULTS_PATH.str_replace($espace, "", $user).'/'.$date.'/'.$XML_FILENAME.'" target="_blank">'.$XML_FILENAME.'</a></p>
		</div>

		<div align="center" class="info span-9">
			<br>
			<p>Here is the output of your OpenAirEmulation :
			<br>
			<a href="'.$RESULTS_PATH.str_replace($espace, "", $user).'/'.$date.'/oaisim.log" target="_blank">oaisim.log</a></p>
		</div>

		'.$PROFILING_OUTPUT.'

		'.$PCAP_OUTPUT.'

		<div class=" span-10">
			<br>
		</div>
</div>
';


/*
// THE SECOND SQUARE  - DISPLAY FILE RESULTS
 if ($handle = opendir("$RESULTS_PATH_EMULATION_LOGS")) {
   while (false !== ($file = readdir($handle)))
      {
          if ($file != "." && $file != "..")
          {
                $thelist .= '<a href="'.$RESULTS_PATH_EMULATION_LOGS.$file.'" target="_blank">'.$file.'</a><br />';
          }
       }
  closedir($handle);
  }

echo '<div align="center" class="success span-20">
<P>List of files:</p>
<P><a href="'.$RESULTS_PATH_EMULATION_LOGS.'oaisim.log.bz2" target="_blank">oaisim.log.bz2</a><br /></p>
</div>';

// THE THIRD SQUARE  - DISPLAY FILE RESULTS
echo '<div align="center" class="success span-20">
<P>HERE DISPLAY PACKET TRACES FILES</p>
</div>';

*/
 


/* PHIL - J'enlve la SUMO CONFIG 
<SUMO_CONFIG> 
				  <SUMO_CMD>'.nospace($_POST['sumocmd']).'</SUMO_CMD>
				  <SUMO_CONFIG_FILE>'.nospace($_POST['sumoconffile']).'</SUMO_CONFIG_FILE>
				  <SUMO_START>'.nospace($_POST['sumostart']).'</SUMO_START>
				  <SUMO_END>'.nospace($_POST['sumoend']).'</SUMO_END>
				  <SUMO_STEP>'.nospace($_POST['sumostep']).'</SUMO_STEP>
				  <SUMO_HOST_IP>'.nospace($_POST['sumohostip']).'</SUMO_HOST_IP>
				  <SUMO_HOST_PORT>'.nospace($_POST['sumohostport']).'</SUMO_HOST_PORT>
				</SUMO_CONFIG>


<CLI>
			    <START_ENB>'.$CLI_START_ENB.'</START_ENB>
			    <START_UE>'.$CLI_START_UE.'</START_UE>
			</CLI>

<NB_ANTENNAS_RX>'.nospace($_POST['eNB_antennas_rx']).'</NB_ANTENNAS_RX>
				<NB_ANTENNAS_TX>'.nospace($_POST['eNB_antennas_tx']).'</NB_ANTENNAS_TX>

<NB_ANTENNAS_RX>'.nospace($_POST['UE_antennas_rx']).'</NB_ANTENNAS_RX>
				<NB_ANTENNAS_TX>'.nospace($_POST['UE_antennas_tx']).'</NB_ANTENNAS_TX>
*/



/*
// UE MOBILITY TYPE
//
if ( $_POST['uemobility'] == 'fixed' ) {
$UE_MOBILITY	=
	'<UE_MOBILITY_TYPE>'.nospace($_POST['uemobility']).'</UE_MOBILITY_TYPE>';
		}		
if ( $_POST['uemobility'] == 'random_waypoint' ) {
$UE_MOBILITY	=
	'<UE_MOBILITY_TYPE>'.nospace($_POST['uemobility']).'</UE_MOBILITY_TYPE>';
		}
if ( $_POST['uemobility'] == 'random_walk' ) {
$UE_MOBILITY	=
	'<UE_MOBILITY_TYPE>'.nospace($_POST['uemobility']).'</UE_MOBILITY_TYPE>';
		}
if ( $_POST['uemobility'] == 'grid_walk' ) {
$UE_MOBILITY	=
	'<UE_MOBILITY_TYPE>'.nospace($_POST['uemobility']).'</UE_MOBILITY_TYPE>
	<GRID_MAP>
		<GRID_HORIZONTAL_NUMBER>'.nospace($_POST['horizgrid']).'</GRID_HORIZONTAL_NUMBER>
		<GRID_VERTICAL_NUMBER>'.nospace($_POST['vertgrid']).'</GRID_VERTICAL_NUMBER>
	</GRID_MAP>
	<GRID_TRIP_TYPE>'.nospace($_POST['gridtype']).'</GRID_TRIP_TYPE>';
		}

//
// UEs DISTRIBUTION
//
if ( $_POST['uedistr'] == 'random' ) {
$UE_DISTRIBUTION	=
	'<RANDOM_UE_DISTRIBUTION>
					<NUMBER_OF_NODES>1</NUMBER_OF_NODES>
				</RANDOM_UE_DISTRIBUTION>';
		}
		
if ( $_POST['uedistr'] == 'concentrated' ) {
$UE_DISTRIBUTION	=
	'<CONCENTRATED_UE_DISTRIBUTION>
					<NUMBER_OF_NODES>1</NUMBER_OF_NODES>
				</CONCENTRATED_UE_DISTRIBUTION>';
		}
		
if ( $_POST['uedistr'] == 'grid_map' ) {
	if ( $_POST['gridmapradio'] == 'random_grid' ) {
$UE_DISTRIBUTION	=
	'<GRID_UE_DISTRIBUTION>random_grid</GRID_UE_DISTRIBUTION>
		<RANDOM_GRID>
			<NUMBER_OF_NODES>'.nospace($_POST['rdgridnbnodes']).'</NUMBER_OF_NODES>
		</RANDOM_GRID>';
	}	
	if ( $_POST['gridmapradio'] == 'border_grid' ) {
$UE_DISTRIBUTION	=
	'<GRID_UE_DISTRIBUTION>border_grid</GRID_UE_DISTRIBUTION>
					<BORDER_GRID>
						<NUMBER_OF_NODES>'.nospace($_POST['bdgridnbnodes']).'</NUMBER_OF_NODES>
					</BORDER_GRID>';
	}		
}

//
// eNB INITIAL DISTRIBUTION 
//
if ( $_POST['eNB_initial_distribution'] == 'grid' ) {
$eNB_INITIAL_DISTRIBUTION	=
	'<eNB_INITIAL_DISTRIBUTION>grid</eNB_INITIAL_DISTRIBUTION>
		<GRID_eNB_DISTRIBUTION>
			<NUMBER_OF_GRID_X>'.nospace($_POST['gridx']).'</NUMBER_OF_GRID_X>
			<NUMBER_OF_GRID_Y>'.nospace($_POST['gridy']).'</NUMBER_OF_GRID_Y>
		</GRID_eNB_DISTRIBUTION>';
		}
if ( $_POST['eNB_initial_distribution'] == 'hexagonal' ) {
$eNB_INITIAL_DISTRIBUTION	=	
	'<eNB_INITIAL_DISTRIBUTION>hexagonal</eNB_INITIAL_DISTRIBUTION>
		<HEXAGONAL_eNB_DISTRIBUTION>
			<NUMBER_OF_CELLS>'.nospace($_POST['hexanbcells']).'</NUMBER_OF_CELLS>
			<INTER_eNB_DISTANCE>'.nospace($_POST['interenbdist']).'</INTER_eNB_DISTANCE>
		</HEXAGONAL_eNB_DISTRIBUTION>';
		}
if ( $_POST['eNB_initial_distribution'] == 'random' ) {
$eNB_INITIAL_DISTRIBUTION	=	
	'<eNB_INITIAL_DISTRIBUTION>random</eNB_INITIAL_DISTRIBUTION>
		<RANDOM_eNB_DISTRIBUTION>
			<NUMBER_OF_CELLS>'.nospace($_POST['nbenbs']).'</NUMBER_OF_CELLS>
		</RANDOM_eNB_DISTRIBUTION>';
		}					
			
//
// TRAFFIC - PACKET SIZE 
//
if ( $_POST['pktsize'] == 'fixed' ) {
$PACKET_SIZE	=
	'<PACKET_SIZE>fixed</PACKET_SIZE>
			<FIXED_PACKET_SIZE>
				<FIXED_VALUE>'.nospace($_POST['pktsizefixed']).'</FIXED_VALUE>
			</FIXED_PACKET_SIZE>';
		}
if ( $_POST['pktsize'] == 'uniform' ) {
$PACKET_SIZE	=	
	'<PACKET_SIZE>uniform</PACKET_SIZE>
				<UNIFORM_PACKET_SIZE>
					<MIN_VALUE>'.nospace($_POST['pktsizeunimin']).'</MIN_VALUE>
					<MAX_VALUE>'.nospace($_POST['pktsizeunimax']).'</MAX_VALUE>
				</UNIFORM_PACKET_SIZE>';
		}
		
//
// INTER ARRIVAL TIME
//
if ( $_POST['interarrivtime'] == 'fixed' ) {
$INTER_ARRIVAL_TIME	=
	'<INTER_ARRIVAL_TIME>fixed</INTER_ARRIVAL_TIME>
				<FIXED_INTER_ARRIVAL_TIME>
					<FIXED_VALUE>'.nospace($_POST['intarrtimefixed']).'</FIXED_VALUE>
				</FIXED_INTER_ARRIVAL_TIME>';
		}
if ( $_POST['interarrivtime'] == 'uniform' ) {
$INTER_ARRIVAL_TIME	=	
	'<INTER_ARRIVAL_TIME>uniform</INTER_ARRIVAL_TIME>
				<UNIFORM_INTER_ARRIVAL_TIME>
					<MIN_VALUE>'.nospace($_POST['intarrtimeunimin']).'</MIN_VALUE>
					<MAX_VALUE>'.nospace($_POST['intarrtimeunimax']).'</MAX_VALUE>
				</UNIFORM_INTER_ARRIVAL_TIME>';
		}
if ( $_POST['interarrivtime'] == 'poisson' ) {
$INTER_ARRIVAL_TIME	=	
	'<INTER_ARRIVAL_TIME>poisson</INTER_ARRIVAL_TIME>
				<POISSON_INTER_ARRIVAL_TIME>
					<EXPECTED_INTER_ARRIVAL_TIME>'.nospace($_POST['intarrtimepoisson']).'</EXPECTED_INTER_ARRIVAL_TIME>
				</POISSON_INTER_ARRIVAL_TIME>';
		}			

//
// RANDOM SEED
//
if ( $_POST['randomseed'] == 'random' ) {
$RANDOM_SEED	=	
	'<SEED>random</SEED>';
		}			
if ( $_POST['randomseed'] == 'userspecified' ) {
$RANDOM_SEED	=	
	'<SEED>user_specified</SEED>
		<USER_SEED>
			<SEED_VALUE>'.nospace($_POST['seedvalue']).'</SEED_VALUE>
		</USER_SEED>';
		}		
*/	
?>
