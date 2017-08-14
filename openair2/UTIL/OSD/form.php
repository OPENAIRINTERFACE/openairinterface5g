<blockquote>
* All fields have to be filled in. If you get a red cross <img src="/OpenAirEmu/css/images/false.png" /> it means your field is empty, so please insert a value. Whereas if you get a green tick <img src="/OpenAirEmu/css/images/true.png" />, it would mean that you correctly filled in the field<br>
* The fields in grey have been disabled for the moment since they are not yet supported by our Emulation plateform.<br><br>
</blockquote>

<form method="post" action="recep.php" >

<!-- Hidden Values-->
<input type="hidden" value="root " name="username" />
<input type="hidden" value="0" name="executed" />
	
<div class="success span-30 last">
	<fieldset>
		<legend>Environment and System Configuration</legend>
		<div class="span-29 inline">
			<div class="span-6">
				<h4><u>Fading</u></h4>
				<label>Pathloss model</label><br>
				   	Pathloss Exponent :<input name="pathlossexp" id="pathlossexp" size="2" maxlength="4" value="2.0" class="required"> (2 to 6)<span class="message"></span><br>
					Pathloss Zero Db: <input name="pathlosszero" id="pathlosszero" size="2" maxlength="4" value="-50" class="required"> dB (0 to -150)<span class="message"></span><br><br>
			</div>	
			<div class="span-4">
			<br><br><label>Small Scale</label><br>
					<input type="radio" name="smallscale" value="SCM_A">SCM_A<br>
					<input type="radio" name="smallscale" value="SCM_B">SCM_B<br>
					<input type="radio" name="smallscale" value="SCM_C">SCM_C<br>
					<input type="radio" name="smallscale" value="SCM_D">SCM_D<br>
					<input type="radio" name="smallscale" value="EPA">EPA<br>
					<input type="radio" name="smallscale" value="EVA">EVA<br>
					<input type="radio" name="smallscale" value="ETU">ETU<br>
					<input type="radio" name="smallscale" value="Rayleigh8">Rayleigh8<br>
					<input type="radio" name="smallscale" value="Rayleigh1">Rayleigh1<br>
					<input type="radio" name="smallscale" value="Rayleigh1_corr">Rayleigh1_corr<br>
					<input type="radio" name="smallscale" value="Rayleigh1_anticorr">Rayleigh1_anticorr<br>
					<input type="radio" name="smallscale" value="Rice8">Rice8<br>
					<input type="radio" name="smallscale" value="Rice1">Rice1<br>
					<input type="radio" name="smallscale" value="Rice1_corr">Rice1_corr<br>
					<input type="radio" name="smallscale" value="Rice1_anticorr">Rice1_anticorr<br>
					<input type="radio" name="smallscale" value="AWGN" checked>AWGN<br>
			</div>
			
			<div class="span-7">
			<h4><u>Other parameters</u></h4>
				<label>Wall penetration loss</label><input name="wallpenetloss" id="wallpenetloss" size="3" maxlength="4" value="5" class="required"> dB<span class="message"></span><br>
				<label>System Bandwidth : </label><input name="sysbw" id="sysbw" size="3" maxlength="4" value="5" class="required"> MHz<span class="message"></span><br>
				<label>UE Frequency : </label><input name="uefreq" id="uefreq" size="3" maxlength="4" value="1.9" class="required"> GHz<span class="message"></span><br>
				<label>Number of RBs DL (6,15,25,50,75,100) : </label><input name="num_rbs_dl" id="num_rbs_dl" size="3" maxlength="4" value="25"><span class="message"></span><br>
                <label>Target DL MCS : </label><input name="dl_mcs" id="dl_mcs" size="3" maxlength="4" value="20"><span class="message"></span><br>
                <label>Target UL MCS : </label><input name="ul_mcs" id="ul_mcs" size="3" maxlength="4" value="16"><span class="message"></span><br>
                <label>SNR : </label><input name="snr" id="snr" size="3" maxlength="4" value="15"><span class="message"></span><br>
                <label>Transmission Mode (1,2,5,6) : </label><input name="transmission_mode" id="transmission_mode" size="3" maxlength="4" value="1"><span class="message"></span><br>
                <br><label>Frame Type : </label><br>
                <input type="radio" name="frame_type" value="1" checked>TDD&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<label>Frame Config (0-6) : </label><input name="tdd_config" id="tdd_config" size="3" maxlength="4" value="1"><span class="message"></span><br>
                <input type="radio" name="frame_type" value="0">FDD<br>			
			</div>
			
			<div class="span-10">
				<div class="span-5">
				<h4><u>Antenna</u></h4>
					<label>eNB Antenna : </label><br><br>
						<label>RX Noise Power</label><input name="eNB_RX_noise_power" id="eNB_RX_noise_power" size="1" maxlength="4" value="5" class="required"> dB<span class="message"></span><br>
						<label>Number of Sectors :</label><input name="eNB_nb_sectors" id="eNB_nb_sectors" size="1" maxlength="4" value="3" class="required"><span class="message"></span><br>
						<label>Beam Width :</label><input name="eNB_beam_width" id="eNB_beam_width" size="1" maxlength="4" value="1.13" class="required"> dB<span class="message"></span><br>
						<label>Antenna gain</label><input name="eNB_antenna_gain" id="eNB_antenna_gain" size="1" maxlength="4" value="16" class="required"> dBi<span class="message"></span><br>
						<label>TX Power</label><input name="eNB_TX_power" id="eNB_TX_power" size="1" maxlength="4" value="40" class="required"> dBm<span class="message"></span><br>
						<label>Nb antennas RX</label><input name="eNB_antennas_rx" id="eNB_antennas_rx" size="1" maxlength="4" value="1" class="required"><span class="message"></span><br>
						<label>Nb antennas TX</label><input name="eNB_antennas_tx" id="eNB_antennas_tx" size="1" maxlength="4" value="1" class="required"><span class="message"></span><br>
				</div>
				<div class="span-5 last">
					<br><br><label>UE Antenna : </label><br><br>
						<label>RX Noise Power</label><input name="UE_RX_noise_power" id="UE_RX_noise_power" size="1" maxlength="4" value="1" class="required"> dB<span class="message"></span><br>
						<label>Antenna gain</label><input name="UE_antenna_gain" id="UE_antenna_gain" size="1" maxlength="4" value="0" class="required"> dBi<span class="message"></span><br>
						<label>TX Power</label><input name="UE_TX_power" id="UE_TX_power" size="1" maxlength="4" value="20" class="required"> dBm<span class="message"></span><br>
						<label>Nb antennas RX</label><input name="UE_antennas_rx" id="UE_antennas_rx" size="1" maxlength="4" value="1" class="required"><span class="message"></span><br>
						<label>Nb antennas TX</label><input name="UE_antennas_tx" id="UE_antennas_tx" size="1" maxlength="4" value="1" class="required"><span class="message"></span><br>
				</div>
			</div>
		</div>
	</fieldset>
</div>

<div class="info span-25 last">
<fieldset>
	<legend>Topology Configuration</legend>
		<div class="span-24 inline">
		<!-- 1ere ligne -->	
			<div class="span-6">
			<label>Area</label><br>
				Area X : <input name="areax" id="areax" size="4" maxlength="4" value="500" class="required"> m<span class="message"></span><br>
				Area Y : <input name="areay" id="areay" size="4" maxlength="4" value="500" class="required"> m<span class="message"></span><br>
			</div>
			
			<div class="span-6">
			<label>Network Type</label><br>
		        <input type="radio" name="typetopo" value="homogeneous" checked> Homogeneous<br>
		        <input type="radio" name="typetopo" value="heterogeneous"> Heterogeneous<br>
			</div>
			
			<div class="span-6">
			<label>Cell Type</label><br>
				<input type="radio" name="celltype" value="macrocell" checked> Macrocell<br>
				<input type="radio" name="celltype" value="microcell" class="disabled"> MicroCell<br>
				<input type="radio" name="celltype" value="picocell" class="disabled"> PicoCell<br>
				<input type="radio" name="celltype" value="femtocell" class="disabled"> FemtoCell<br>
			</div>
			
			<div class="span-6 last">
			<label>Relay</label><br>
				Number of Relays : <input name="nbrelay" id="nbrelay" size="1" maxlength="4" value="0" class="disabled"> (DISABLED)<span class="message"></span><br>
			</div>
		</div>	

		<div class="span-24 inline">
			<h4><u>Mobility</u></h4>
			<div class="span-5">
			
			<label>UE Initial Distribution : </label><br>
					<input type="radio" name="uedistr" value="random" checked> Random<br>
					<input type="radio" name="uedistr" value="concentrated" class="disabled"> Concentrated<br>
					<input type="radio" name="uedistr" value="grid_map" class="disabled"> Grid Map<br> 
					<span id="gridue" style="display:none"> 
						<div class="span-10">
						<input type="radio" name="gridmapradio" value="random_grid" checked> Random grid : <span id="random_grid" style="display"> Number of Nodes : <input name="rdgridnbnodes" id="rdgridnbnodes" size="1" maxlength="4" value="1" class="required"><span class="message"></span></span><br>
						</div>
						<div class="span-10">
						<input type="radio" name="gridmapradio" value="border_grid"> Border grid : <span id="border_grid" style="display:none"> Number of Nodes : <input name="bdgridnbnodes" id="bdgridnbnodes" size="1" maxlength="4" value="1" class="required"><span class="message"></span></span><br>
						</div>
					</span><br>
			
			<label>Random UE Distribution : </label><br>
				Number of Nodes : <input name="random_UE_dist_nb_node" id="random_UE_dist_nb_node" size="1" maxlength="4" value="1"><span class="message"></span><br><br>
			
			
				<label>UE Mobility Type</label><br>
					<input type="radio" name="uemobility" value="STATIC" checked> Static<br>
					<input type="radio" name="uemobility" value="RWP"> Random Way point<br>
					<input type="radio" name="uemobility" value="RWALK"> Random Walk<br>
					<input type="radio" name="uemobility" value="TRACE"> Trace<br>
					<input type="radio" name="uemobility" value="SUMO" class="disabled"> Sumo<br>
					<input type="radio" name="uemobility" value="grid_walk" class="disabled"> Grid Walk <br><span id="grid_walk" style="display:none"> -> <u>Grid Map :</u> Vertical grid :<input name="vertgrid" id="vertgrid" size="1" maxlength="4" value="1" class="required">
																																					 Horizontal grid :<input name="horizgrid" id="horizgrid" size="1" maxlength="4" value="1" class="required"><br>
					-> <u>Grid Trip Type :</u><br>				 
						<input type="radio" name="gridtype" value="random_destination" checked> Random Destination<br>
						<input type="radio" name="gridtype" value="random_turn"> Random Turn<br>
						<span class="message"></span></span><br>
			</div>
			
			<div class="span-8">
				<label>UE Moving Dynamics : </label><br>
					Min Speed : <input name="minspeed" id="minspeed" size="1" maxlength="4" value="1" class="required"><span class="message"></span><br>
					Max Speed : <input name="maxspeed" id="maxspeed" size="1" maxlength="4"  value="10" class="required"><span class="message"></span><br>
					Min Sleep : <input name="minpausetime" id="minpausetime" size="1" maxlength="4"  value="1" class="required"><span class="message"></span><br>
					Max Sleep : <input name="maxpausetime" id="maxpausetime" size="1" maxlength="4"  value="10" class="required"><span class="message"></span><br>
					Min Journey Time : <input name="minjourneytime" id="minjourneytime" size="1" maxlength="4"  value="1" class="required"><span class="message"></span><br>
					Max Journey Time : <input name="maxjourneytime" id="maxjourneytime" size="1" maxlength="4"  value="10" class="required"><span class="message"></span><br>
				<br>	
				
				<label>SUMO Config : </label><br>
					SUMO CMD : <input name="sumocmd" id="sumocmd" size="3" maxlength="4" value="sumo" class="required"><span class="message"></span><br>
					SUMO CONFIG FILE : <input name="sumoconffile" id="sumoconffile" size="20" maxlength="64"  value="$OPENAIR2_DIR/UTIL/OMG/SUMO/SCENARIOS/scen.sumo.cfg" class="required"><span class="message"></span><br>
					SUMO START : <input name="sumostart" id="sumostart" size="1" maxlength="4"  value="0" class="required"><span class="message"></span><br>
					SUMO END : <input name="sumoend" id="sumoend" size="1" maxlength="4"  value="10" class="required"><span class="message"></span><br>
					SUMO STEP : <input name="sumostep" id="sumostep" size="1" maxlength="4"  value="10" class="required"><span class="message"></span><br>
					SUMO HOST IP : <input name="sumohostip" id="sumohostip" size="10" maxlength="64"  value="127.0.1.1" class="required"><span class="message"></span><br>
					SUMO HOST PORT : <input name="sumohostport" id="sumohostport" size="3" maxlength="6"  value="8883" class="required"><span class="message"></span><br>
				<br>	
				
			</div>
			
			<div class="span-7">
				<label>eNB Initial Distribution : </label><br>
					<input type="radio" name="eNB_initial_distribution" value="fixed" checked> Fixed <br>
                    Pos X : <input name="pos_x" id="pos_x" size="4" maxlength="4" value="250" class="required"> m<span class="message"></span><br>
                    Pos Y : <input name="pos_y" id="pos_y" size="4" maxlength="4" value="250" class="required"> m<span class="message"></span><br>
                    <input type="radio" name="eNB_initial_distribution" value="random"> Random <!--<span id="random" style="display">-> Number of cells :<input name="nbenbs" id="nbenbs" size="1" maxlength="4" value="1" class="required"><span class="message"></span></span>--><br>
                    <input type="radio" name="eNB_initial_distribution" value="grid" class="disabled"> Grid <!-- <span id="grid" style="display:none">-> Grid X value :<input name="gridx" id="gridx" size="1" maxlength="4" value="1" class="required"><span class="message"></span>Grid Y value :<input name="gridy" id="gridy" size="1" maxlength="4" value="1" class="required"><span class="message"></span></span>--><br>
                    <input type="radio" name="eNB_initial_distribution" value="hexagonal" class="disabled"> Hexagonal <!--<span id="hexagonal" style="display:none">-> Number of Cells :<input name="hexanbcells" id="hexabncells" size="1" maxlength="4" value="1" class="required"><span class="message"></span> Inter eNB distance :<input name="interenbdist" id="interenbdist" size="1" maxlength="4" value="1" class="required"> km<span class="message"></span></span>--><br>
                    <br>
				
				<label>Random eNB Distribution : </label><br>
					Number of Cells : <input name="random_eNB_distr" id="random_eNB_distr" size="1" maxlength="4" value="1"><span class="message"></span><br><br>
				
				<label>eNB Mobility Type : </label><br>
					<input type="radio" name="enbmobility" value="STATIC" checked> Static<br>
					<input type="radio" name="enbmobility" value="mobile"> Mobile<br>
					
			</div>
			
			<div class="span-4 last">
			      <label>OMV :</label> <input name="omv" id="omv" size="1" maxlength="4" value="0"><span class="message"></span><br><br>
			</div>
	</div>
</fieldset>	
</div>

<div class="dde span-33 last">
	<fieldset>
		<legend>Application Configuration</legend>
		
		<div class="span-30 inline">
			
			<div class="span-5">
				<label><u>Predefined Traffic :</u></label><br>
					<label>Source ID :</label> <input name="pre_sourceid" id="pre_sourceid" size="2" maxlength="4" value="0"><span class="message"></span><br><br>
					<label>Destination ID :</label> <input name="pre_destid" id="pre_destid" size="2" maxlength="4" value="1"><span class="message"></span><br><br>	
					<label>Background traffic :</label> <input name="bg_traffic" id="bg_traffic" value="0" type="checkbox"><span class="message"></span><br><br>	
					<label>Transport Protocol : </label><br>
				  <input type="radio" name="pre_transprot" value="tcp" checked> TCP<br>
				  <input type="radio" name="pre_transprot" value="udp"> UDP<br><br>
				  <label>IP VERSION : </label><br>
				  <input type="radio" name="pre_ipversion" value="ipv4" checked> IPv4<br>
				  <input type="radio" name="pre_ipversion" value="ipv6"> IPv6<br><br>
					<label>Application Type :</label><br>
					<input type="radio" name="pre_apptype" value="no_predefined_traffic" checked> no customised traffic<br>
					<input type="radio" name="pre_apptype" value="scbr"> scbr<br>
					<input type="radio" name="pre_apptype" value="mcbr"> mcbr<br>
					<input type="radio" name="pre_apptype" value="bcbr"> bcbr<br>
					<input type="radio" name="pre_apptype" value="m2m_AP"> m2m_AP<br>
					<input type="radio" name="pre_apptype" value="m2m_BR"> m2m_BR<br>
					<input type="radio" name="pre_apptype" value="gaming_OA"> gaming_OA<br>
					<input type="radio" name="pre_apptype" value="gaming_TF"> gaming_TF<br>
					<input type="radio" name="pre_apptype" value="full_buffer"> full_buffer<br>
					<input type="radio" name="pre_apptype" value="customised"> customised<br>
					<br>
			</div>						
									
			<div class="span-5">						
									
				<label><u>Customised Traffic :</u></label><br>					
				<label>SOURCE ID : </label><input name="cust_sourceid" id="cust_sourceid" size="2" maxlength="4" value="0"><span class="message"></span><br><br>
				<label>DESTINATION ID : </label><input name="cust_destid" id="cust_destid" size="2" maxlength="4" value="1"><span class="message"></span><br><br>
				<label>Transport Protocol : </label><br>
				  <input type="radio" name="cust_transprot" value="tcp" checked> TCP<br>
				  <input type="radio" name="cust_transprot" value="udp"> UDP<br><br>
				<label>IP VERSION : </label><br>
				  <input type="radio" name="cust_ipversion" value="ipv4" checked> IPv4<br>
				  <input type="radio" name="cust_ipversion" value="ivp6"> IPv6<br>
			</div>
			
			<div class="span-5">
			<br>
			<label>IDT DIST : </label><br>
				  <input type="radio" name="idt_dist" value="no_customized_traffic" checked> No customised traffic<br>
				  <input type="radio" name="idt_dist" value="uniform"> uniform<br>
				  <input type="radio" name="idt_dist" value="poisson" > poisson<br>
				  <input type="radio" name="idt_dist" value="gaussian"> gaussian<br>
				  <input type="radio" name="idt_dist" value="exponential"> exponential<br>
				  <input type="radio" name="idt_dist" value="static"> static<br>
				  <input type="radio" name="idt_dist" value="weibull"> weibull<br>
				  <input type="radio" name="idt_dist" value="pareto"> pareto<br>
				  <input type="radio" name="idt_dist" value="gamma"> gamma<br>
				  <input type="radio" name="idt_dist" value="cauchy"> cauchy<br>
				<label>IDT MIN : </label><input name="idtmin" id="idtmin" size="4" maxlength="4" value="100"> ms<span class="message"></span><br>
				<label>IDT MAX : </label><input name="idtmax" id="idtmax" size="4" maxlength="4" value="1000"> ms<span class="message"></span><br>
				<label>IDT Standard Deviation : </label><input name="idtstddeviation" id="idtstddeviation" size="4" maxlength="4" value="4.5"><span class="message"></span><br>
				<label>IDT Lambda : </label><input name="idtlambda" id="idtlambda" size="4" maxlength="4" value="7.3"><span class="message"></span><br>
				<label>IDT Scale : </label><input name="idtscale" id="idtscale" size="4" maxlength="4" value="30"><span class="message"></span><br>
				<label>IDT Shape : </label><input name="idtshape" id="idtshape" size="4" maxlength="4" value="15"><span class="message"></span><br>
			</div>
			
				
			<div class="span-5">
			<br>
			<label>SIZE DIST : </label><br>
			 <input type="radio" name="size_dist" value="no_customized_traffic" checked> No customised traffic<br>
			  <input type="radio" name="size_dist" value="uniform" > uniform<br>
			  <input type="radio" name="size_dist" value="poisson"> poisson<br>
			  <input type="radio" name="size_dist" value="gaussian"> gaussian<br>
			  <input type="radio" name="size_dist" value="exponential"> exponential<br>
			  <input type="radio" name="size_dist" value="static"> static<br>
			  <input type="radio" name="size_dist" value="weibull"> weibull<br>
			  <input type="radio" name="size_dist" value="pareto"> pareto<br>
			  <input type="radio" name="size_dist" value="gamma"> gamma<br>
			  <input type="radio" name="size_dist" value="cauchy"> cauchy<br>
			<label>SIZE MIN byte : </label><input name="sizemin" id="sizemin" size="4" maxlength="4" value="100"><span class="message"></span><br>
			<label>SIZE MAX byte : </label><input name="sizemax" id="sizemax" size="4" maxlength="4" value="500"><span class="message"></span><br>
			<label>SIZE Std Deviation : </label><input name="sizestddeviation" id="sizestddeviation" size="4" maxlength="4" value="5.3"><span class="message"></span><br>
			<label>SIZE Lambda : </label><input name="sizelambda" id="sizelambda" size="4" maxlength="4" value="2.2"><span class="message"></span><br>
			<label>SIZE Scale : </label><input name="sizescale" id="sizescale" size="4" maxlength="4" value="0.5"><span class="message"></span><br>
			<label>SIZE Shape : </label><input name="sizeshape" id="sizeshape" size="4" maxlength="4" value="30"><span class="message"></span><br>
			<label>Destination Port : </label><input name="destport" id="destport" size="4" maxlength="4" value="1"><span class="message"></span><br>	
			
			</div>
			
			<div class="span-10 last">
					<br>
					<label>PU</label><br>
						PROB OFF-PU : <input name="proboffpu" id="proboffpu" size="3" maxlength="4" value="0.7"><span class="message"></span><br>
						PROB PU-ED : <input name="probpued" id="probpued" size="3" maxlength="4" value="0.2"><span class="message"></span><br>
						HOLDING TIME OFF-PU : <input name="holdtimeoffpu" id="holdtimeoffpu" size="3" maxlength="4" value="50"><span class="message"></span><br><br>
									

					<label>ED</label><br>
						PROB OFF-ED : <input name="proboffed" id="proboffed" size="3" maxlength="4" value="0.2"><span class="message"></span><br>
						PROB ED-PE : <input name="probedpe" id="probedpe" size="3" maxlength="4" value="0.1"><span class="message"></span><br>
						HOLDING TIME OFF-ED : <input name="holdtimeoffed" id="holdtimeoffed" size="4" maxlength="4" value="1000"><span class="message"></span><br><br>					
									
					<label>PE</label><br>
						HOLDING TIME OFF-PE : <input name="holdtimeoffpe" id="holdtimeoffpe" size="3" maxlength="4" value="0.1"><span class="message"></span><br>
			</div>
			
			
		</div>			
	</fieldset>
</div>

<div class="alert span-25 last">
    <fieldset>
    <legend>Emulation Configuration</legend>
	
	<div class="span-24 inline">
		<label>Emulation Time</label> : <input name="emutime" id="emutime" size="6" maxlength="6" value="500" class="required"> milliseconds(ms)<span class="message"></span><br>
	</div>
		
	<div class="span-24 prepend-top inline">	
		<label><u>Performance Metrics :</u></label>
	</div>	
		
	<div class="span-24 inline">
		<div class="span-7">
			<input name="throughput" value="1"  type="checkbox" class="required" > Throughput<span class="message"></span><br>
			<input name="latency" value="1"  type="checkbox" class="required" > Latency<span class="message"></span><br>
			<input name="lossrate" value="1" type="checkbox" class="required"> Loss Rate<span class="message"></span><br>
			<br>
            <input name="phy_abstraction" value="1" type="checkbox" class="required"> PHY Abstraction<span class="message"></span><br>
            <input name="profiling" value="1" type="checkbox" class="required" checked> Profiling Statistics<span class="message"></span><br>
            <input name="pcap_trace" value="1" type="checkbox" class="required"> PCAP Trace<span class="message"></span><br>
            <br>
            <input name="cli_enabled" value="1" type="checkbox" class="required"> CLI Enabled<span class="message"></span><br>
            <input name="cli_start_enb" value="1" type="checkbox" class="required"> CLI Start eNB<span class="message"></span><br>
            <input name="cli_start_ue" value="1" type="checkbox" class="required"> CLI Start UE<span class="message"></span><br>
            <br>
			<input name="curves" value="1" type="checkbox" class="required"> Show Curves (not working)<span class="message"></span><br>
		</div>	
		<div class="span-6">
			<label>LOG :</label><br> 
			Level <input name="level" id="level" size="6" maxlength="6" value="debug"><span class="message"></span><br>
			Verbosity <input name="verbosity" id="verbosity" size="6" maxlength="6" value="low"><span class="message"></span><br>
			Interval <input name="interval" id="interval" size="6" maxlength="6" value="1"><span class="message"></span><br>
		</div>
		<div class="span-10 last">
			<label>PACKET TRACE :</label><input name="pkttrace" value="0" type="checkbox"><br>
			<label>SEED VALUE :</label><input name="seedvalue" id="seedvalue" size="1" maxlength="4" value="1234"><span class="message"></span><br><br>
		
		</div>
	</div>
	</fieldset>
</div>

<div align="center" class="notice span-25 last">
Send details : <input type="submit" value="Generate Scenario and Run Emulation" id="envoyer"/><br />
</div>
</form>
