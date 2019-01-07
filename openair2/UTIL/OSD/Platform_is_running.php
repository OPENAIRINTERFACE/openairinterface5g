<?php 
function givePID($pid)
	{
		// create our system command
		$GIVE_PID=shell_exec("pgrep $pid");
		echo $GIVE_PID;
	}
?>

<div id="xmlwritten" class="alert"> 
<p><img src="/OpenAirEmu/css/images/warning.png" /><b>Warning<b/><p />

<p>The OpenAirInterface Simulator is already running.<br /> 
You should wait for the current simulation to be over before to process to a new one.<p/>
If the process is stuck, execute the following command :<br />
<blockquote>" kill <? givePID("oaisim");?> "</blockquote>
</div>
