<!DOCTYPE html>
<HTML>

<HEAD>

<title>LoRa&trade; Mote</title>
<link rel="stylesheet" href="lorastyle.css">

</HEAD>
<BODY>

<?php
ini_set("memory_limit","25M");
include('constants.php');
include('functions.php');

//Read input
if(!isset($_GET['euiText']))
{
	echo "Unable to read mote EUI";
	goto endBody;
}

$moteEui = RemoveNonNumericCharacters($_GET['euiText'],1); 	//strip non hex characters from input
$euiFormatted = FormatEui($moteEui ,':');


//Set LED, if required
if (isset($_GET['led']))
{
	SetMoteLED($moteEui, $_GET['led']);
}

// Query database
//Open connection - then query and close
$appDbConnection = SqlConnect(APPLICATION);
if (!$appDbConnection)
{
	echo "Unable to connect to application database";
	return;
}

$customerDbConnection = SqlConnect(CUSTOMER);
if (!$customerDbConnection)
{
	echo "Unable to connect to customer database";
	return;
}

$query = "SELECT HEX(appEui) AS appEui FROM motes WHERE HEX(eui) = \"$moteEui\"";
$queryResult = mysqli_query($appDbConnection, $query) or die(mysqli_error($appDbConnection));
$rowarray = mysqli_fetch_array($queryResult);
$appEuiUnformatted = $rowarray['appEui'];

if (!empty($appEuiUnformatted))
	$appEui = FormatEui($appEuiUnformatted);
else
	$appEui = "0";
mysqli_free_result($queryResult);

$query = "SELECT HEX(networkAddress) AS networkAddress FROM activemotes WHERE HEX(eui) = \"$moteEui\"";
$queryResult =  mysqli_query($appDbConnection, $query) or die(mysqli_error($appDbConnection));
$rowarray = mysqli_fetch_array($queryResult);
$networkAddress = FormatNetworkAddress($rowarray['networkAddress']);
mysqli_free_result($queryResult);

$query = "SELECT name, owner FROM applications WHERE HEX(eui) = \"$appEuiUnformatted\"";
$queryResult = mysqli_query($appDbConnection, $query) or die(mysqli_error($appDbConnection));
$rowarray = mysqli_fetch_array($queryResult);
$appName = $rowarray['name'];
$appOwner = $rowarray['owner'];
mysqli_free_result($queryResult);

$query = "SELECT time FROM appdata INNER JOIN activemotes ON activemotes.lastRxFrame = appdata.id WHERE HEX(activemotes.eui) = \"$moteEui\"";
$queryResult = mysqli_query($customerDbConnection, $query) or die(mysqli_error($appDbConnection));
$rowarray = mysqli_fetch_array($queryResult);
$lastFrameTime = $rowarray['time'];
mysqli_free_result($queryResult);

mysqli_close($appDbConnection);
mysqli_close($customerDbConnection);
?>
	
<style>
	table {border:0px}
</style>

<table><!-- Outer table -->
<tr>
<td>
	<table><!-- Title table -->
<tr>

<td width = "70%" valign = "center" height = "150">
<h1>LoRa Mote 
	<?php
	echo $euiFormatted;
	?>
</h1>
<br>
<?php
//Print current time (GMT)
$time = gmdate(DATE_RFC1123);
echo $time;
echo " GMT\n";
?>
</td>

<td width = "30%" valign = "top" align = "right"><a href = "demonstrator.php"><img src = "LoRa_square.png" alt = "LoRa from Semtech" width = "150" height = "150"></a></td>
</tr>
</table><!-- Title table -->
</td>
</tr>
<tr><td>
<table><!-- Details table -->
<col width=200px>	
    <tr valign = "top" >
        <td><b>Node</b><br>
		EUI:<br>
		Network Address:<br>
		
		</td>
		<td align="right">
<?php
			nl();
			echo $euiFormatted; nl();
			echo $networkAddress;
?>
		</td>
	</tr>
	<tr valign = "top" >
		<td>
		<b><br>Application</b><br>
		EUI:<br>
		Name:<br>
		Owner:<br>
		Last frame time:<br>
		</td>
		<td align="right">
		<br>
<?php
		nl();
		echo $appEui; nl();
		echo $appName; nl();
		echo $appOwner; nl();
		echo $lastFrameTime . " GMT"; 
?>
		</td>
	</tr>

</table><!-- details table -->
</td></tr>

<tr><!-- button row -->
<td>
<table> <!-- button table-->

<style>
	td {padding:10px}
	p {}
	button {width:100px}
</style>

<tr>
<td>
<form action = "appdatalist.php" method='get' id = "appDataForm">

<?php
echo "<input type =\"hidden\" name=\"euiText\" value = \"$moteEui\">";
?>
</form>
<button type= "submit" form = "appDataForm" name="appDataButton" value="Submit">Application data list</button>

</td>
<td>
<form action = "appdatalistCSV.php" method='get' id = "appDataFormCsv">

<?php
echo "<input type =\"hidden\" name=\"euiText\" value = \"$moteEui\">";
?>
</form>
<button type= "submit" form = "appDataFormCsv" name="appDataButtonCsv" value="Submit">Application data list CSV</button>
</td>
</tr>

<tr>
<td>
<form action = "transmissionperformance.php" method='get' id = "txForm">

<?php
echo "<input type =\"hidden\" name=\"euiText\" value = \"$moteEui\">";
?>
</form>
<button type= "submit" form = "txForm" name="txDataButton" value="Submit">Transmission performance</button>
</td>
</tr>

<tr>
<td>
<form action = "" method= "get">  
<button type = "submit" value = "on" name = "led" width = "70px" height = "50px" value = "1"> Set mote LED on</button>
<input type = "hidden" name = "euiText" 
<?php
	echo " value=\"$moteEui\">";
?>
</form>
</td>
<td>
<form action = "" method= "get">  
<button type = "submit" value = "off" name = "led" width = "70px" height = "50px" value = "0"> Set mote LED off</button>
<input type = "hidden" name = "euiText" 
<?php
	echo " value=\"$moteEui\">";
?>
</form>
</td>
</tr>
</table><!-- button table-->
</td>
</tr>
</table><!-- Outer table -->
<?php
endBody:
?>
</BODY>

</HTML>