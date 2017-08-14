<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
  "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en" dir="ltr"> 
<head>
<?
include('header.php'); 
?>                   
</head>

<body>
<br>
<h3>OpenAirEmulation Scenario Descriptor - Basic Scenario</h3>

<?
// We test here if OpenairConfigGenerator (OCG) and OpenAirEmu are running before to allow the user to use the form

	function checkPid($pid)
	{
		// create our system command
		$cmd = "pgrep $pid";

		// run the system command and assign output to a variable ($output)
		exec($cmd, $output, $result);

		// check the number of lines that were returned
		if(count($output) >= 1){
		  // the process is still running
		  include('Platform_is_running.php');
		  return false;		
		}

		include('form.php');
		return true;
		// the process is dead
	}

// Way Number one - USING the OCG.pid file
/* $myFile = "OCG.pid"; 
$fh = fopen($myFile, 'r');
$theData = fread($fh, filesize($myFile));
fclose($fh);
checkPid($theData); */

// Way Number Two - Checking whether a process called OCG is running
$name = 'oaisim';
checkPid($name);
?>

</body>
</html>

