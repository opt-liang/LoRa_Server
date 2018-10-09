<!DOCTYPE html>
<HTML>

<HEAD>

<title>LoRa&trade; Mote list</title>
<meta http-equiv="refresh" content="10">
<link rel="stylesheet" href="lorastyle.css">
</HEAD>
<BODY>

<?php
ini_set("memory_limit","25M");
include('constants.php');
include('functions.php');



// Query database
//Open connection - then query and close
$custDbConnection = SqlConnect(CUSTOMER);
if (!$custDbConnection)
{
	echo "Unable to connect to customer database";
	goto endBody;
}
$customerQuery = "SELECT HEX(motes.eui) as mote, appdata.time AS lastFrameTime, timediff(UTC_TIMESTAMP(), appdata.time) as timeSinceFrame FROM activemotes as motes LEFT JOIN appdata on motes.lastRxFrame = appdata.id WHERE appdata.time >= DATE_SUB(UTC_TIMESTAMP(), INTERVAL " . ACCEPT_MARGIN .")";
$customerQueryResult = mysqli_query($custDbConnection, $customerQuery) or die(mysqli_error($custDbConnection));
mysqli_close($custDbConnection);

$appDbConnection = SqlConnect(APPLICATION);
if (!$appDbConnection)
{
	echo "Unable to connect to application database";
	goto endBody;
}

?>

<table border = "0"> <!-- Outer table -->
<table border = "0"> <!-- Title table -->
<tr>

<td width = "250px" valign = "top" align = "right"><a href = "demonstrator.php"><img src = "LoRa_square.png" alt = "LoRa from Semtech" width = "150" height = "150" border = 0></a></td>

<td width = "100px" valign = "center" align = "left"> </td>

<td width = "300px" valign = "bottom">
	
<?php
//Print current time (GMT)
$time = gmdate(DATE_RFC1123);
echo "$time GMT";
?>

</td>

</tr>
<tr>
</table> <!-- Title table -->

<table> <!-- Data table holder-->

<!-- Display heading -->
<h2>LoRa mote list</h2>

<table class="datatable"><!-- Data table -->
<col class="datatable" width=120px style=\"background-color: #CCCCCC;\">
<col class="datatable" width=120px>
<col class="datatable" width = 150px>
<col class="datatable">

<tr class=\"datatable\">
<th  class=\"datatable\"> Mote EUI</th>
<th class=\"datatable\"> Mote network address</th>
<th class=\"datatable\"> Time of last frame</th>
<th class=\"datatable\"> Time since last frame</th>
</tr>


<?php
//Display motes (if any)
while($customerRowArray = mysqli_fetch_array($customerQueryResult))
{
	$timeSinceFrameText = $customerRowArray['timeSinceFrame'];
	
	$timeSinceFrameArray = ReadTime($timeSinceFrameText);
	
	$timeSinceFrameText = WriteTime($timeSinceFrameArray);
	$integerSeconds = IntegerSeconds($timeSinceFrameArray);
	
	$colour = FindColour($integerSeconds);

	$moteEui = $customerRowArray['mote'];
	$formattedMoteEui = FormatEui($moteEui);
	$lastFrameTime = $customerRowArray['lastFrameTime'];
	
	$applicationQuery = "SELECT networkAddress from activemotes WHERE HEX(eui) = \"$moteEui\"";
	$applicationQueryResult = mysqli_query($appDbConnection, $applicationQuery) or die(mysqli_error($appDbConnection));
	
	if (!$applicationRowArray = mysqli_fetch_array($applicationQueryResult))
			goto endBody;
	
	$networkAddress = FormatNetworkAddress($applicationRowArray['networkAddress']);
	mysqli_free_result($applicationQueryResult);
	
	//prints row per entry
	echo "<tr height = \"20px\" valign = \"top\">
		<td class=\"datatable\" style=\"text-align:center\"> 
		<a href=\"mote.php?euiText=$moteEui\"> $formattedMoteEui</a>
		</td>
		
		<td  class=\"datatable\"> $networkAddress</td>
		<td  class=\"datatable\" align=\"center\"> <a HREF=\"appdatalist.php?euiText=$moteEui\">$lastFrameTime</A>  </td>
		<td  class=\"datatable\" style=\"text-align:right\"> <span style=\"color: $colour \"> $timeSinceFrameText </span> </td>
		</tr>";
}

mysqli_free_result($customerQueryResult);
mysqli_close($appDbConnection);

?>

</table>
</tr>
<tr>
<br>
<br>
</tr>
</div>
<?php
endBody:
?>
</BODY>

</HTML>