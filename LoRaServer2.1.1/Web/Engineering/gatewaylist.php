<!DOCTYPE html>
<HTML>

<HEAD>

<title>LoRa&trade; Gateway list</title>
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
$connection = SqlConnect(CUSTOMER);

if (!$connection)
{
	echo "Unable to connect to database";
	goto endBody;
}

$query = "SELECT HEX(gateways.eui) as gateway, appdata.time AS lastFrameTime, timediff(UTC_TIMESTAMP(), appdata.time) as timeSinceFrame FROM activegateways as gateways LEFT JOIN appdata on gateways.lastRxFrame = appdata.id WHERE appdata.time >= DATE_SUB(UTC_TIMESTAMP(), INTERVAL " . ACCEPT_MARGIN .")";
$queryResult = mysqli_query($connection, $query) or die(mysqli_error($connection));
mysqli_close($connection);

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
<h2>LoRa gateway list</h2>


<table class="datatable"><!-- Data table -->
<col class="datatable" width=120px style=\"background-color: #CCCCCC;\">
<col class="datatable" width=190px>
<col class="datatable" width=190px>
<col class="datatable">

<tr class=\"datatable\">
<th  class=\"datatable\"> Gateway EUI</th>
<th class=\"datatable\"> Time of last frame</th>
<th class=\"datatable\"> Time since last frame</th>
</tr>


<?php
//Display gateways (if any)
while($rowarray = mysqli_fetch_array($queryResult))
{
	$timeSinceFrameText = $rowarray['timeSinceFrame'];
	
	$timeSinceFrameArray = ReadTime($timeSinceFrameText);
	
	$timeSinceFrameText = WriteTime($timeSinceFrameArray);
	$integerSeconds = IntegerSeconds($timeSinceFrameArray);
	
	$colour = FindColour($integerSeconds);

	$gatewayEui = $rowarray['gateway'];
	$formattedGatewayEui = FormatEui($gatewayEui);
	$lastFrameTime = $rowarray['lastFrameTime'];
	
	//prints row per entry
	echo "<tr height = \"20px\" valign = \"top\">
		<td class=\"datatable\" align=\"center\"> 
		<a href=\"framelistbygateway.php?euiText=$gatewayEui\">$formattedGatewayEui</a>
		</td>
		
		<td class=\"datatable\" align=\"center\"> ". $rowarray['lastFrameTime']. " </td>
		<td class=\"datatable\" style=\"text-align:right\"> <span style=\"color: $colour \"> $timeSinceFrameText </span> </td>
		</tr>";
}

mysqli_free_result($queryResult);

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