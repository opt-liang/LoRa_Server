<!DOCTYPE html>
<HTML>

<HEAD>

<title>LoRa&trade; Frame list by gateway</title>
<meta http-equiv="refresh" content="10">
<link rel="stylesheet" href="lorastyle.css">
</HEAD>
<BODY>

<?php
ini_set("memory_limit","25M");
include('constants.php');
include('functions.php');

//Interpret input
if(!isset($_GET['euiText']))
{
	echo "Unable to read gateway EUI";
	goto endBody;
}

$gatewayEui = RemoveNonNumericCharacters($_GET['euiText'], true);
$gatewayFormatted = FormatEui($gatewayEui ,':');

if(isset($_GET['packetNumberText']))
	$numberOfPackets = RemoveNonNumericCharacters($_GET['packetNumberText'],0); 	//strip non decimal characters from input
else
	$numberOfPackets = MAX_PACKETS_DISPLAYED;

// Query database
//Open connection - then query and close
$connection = SqlConnect(CUSTOMER);

if (!$connection)
{
	echo "Unable to connect to database";
	goto endBody;
}
$query = "SELECT HEX(a.mote) as mote, HEX(seqNo) as seqNo, signalToNoiseRatio_cB, loraAdrEnabled, channel, frequency, loraModulation, loraModulationBandwidth_Hz, loraCodingRateNumerator, loraCodingRateDenominator, loraSpreadingFactor, fskBitRate, rank, receiveTime, receiveTime_nsec, rfChain, signalStrength_dBm, signalToNoiseRatio_cB FROM gatewayframerx AS g JOIN appdata AS a ON g.id = a.id  JOIN moteframetx as t ON g.id = t.id WHERE HEX(gatewayEui) = \"$gatewayEui\" ORDER BY a.id DESC LIMIT $numberOfPackets";
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

<?php
//Display heading
echo "<h2>LoRa frames received by Gateway $gatewayFormatted</h2>";

?>

<table class="datatable"><!-- Data table -->
<col class="datatable" style="width:120px"> <!-- Sequence -->
<col class="datatable" style="width:250px"> <!-- Time -->
<col class="datatable" style="width:180px"> <!-- Mote -->
<col class="datatable" style="width:40px"> <!-- Frequency -->
<col class="datatable" style="width:40px"> <!-- Modulation -->
<col class="datatable" style="width:40px"> <!-- Modulation bandwidth -->
<col class="datatable" style="width:40px"> <!-- Spreading factor -->
<col class="datatable" style="width:40px"> <!-- Coding rate -->
<col class="datatable" style="width:40px"> <!-- ADR -->
<col class="datatable" style="width:40px"> <!-- Channel -->
<col class="datatable" style="width:40px;"> <!-- Signal strength -->
<col class="datatable" style="width:40px;"> <!-- Signal to noise ratio -->

<tr class="datatable">
<th> Sequence #</th>
<th> Time</th>
<th> Mote</th>
<th> Frequency (MHz)</th>
<th> Modulation </th>
<th> LoRa Modulation bandwidth (Hz) or FSK bit rate (bit/s)</th>
<th> Spreading factor </th>
<th> Coding rate </th>
<th> Adaptive data rate </th>
<th> Channel </th>
<th> Signal strength (dBm) </th>
<th> Signal to noise ratio (dB) </th>
</tr>

<?php
//Display frames (if any)
while($rowarray = mysqli_fetch_array($queryResult))
{
	$seqNo = $rowarray['seqNo'];
	$unformattedMoteEui = $rowarray['mote'];
	$formattedMoteEui = FormatEUI($unformattedMoteEui);
	$frequency = $rowarray['frequency'] / 1e6;
	$loraModulation = $rowarray['loraModulation'];
	
	
	$bandwidth_Hz = $loraModulation ? ($rowarray['loraModulationBandwidth_Hz'] . "Hz") : ($rowarray['fskBitRate'] . "bit/s");
		
	if ($loraModulation)
	{
		$loraModulationText = "LoRa";
		$bandwidth_Hz = $rowarray['loraModulationBandwidth_Hz'];
		$spreadingFactor = "SF" . $rowarray['loraSpreadingFactor'];
		$codingRateNumerator = $rowarray['loraCodingRateNumerator'];
		$codingRateDenominator = $rowarray['loraCodingRateDenominator'];
		$codingRate = $codingRateNumerator . "/" . $codingRateDenominator;
		$loraAdrEnabled = $rowarray['loraAdrEnabled'] ? "on" : "off";
	}
	else
	{
		$loraModulationText = "FSK";
		$bandwidth_Hz = $rowarray['fskBitRate'];
		$spreadingFactor = "";
		$codingRate = "";
		$loraAdrEnabled = "";
	}
	
	$receiveTime = $rowarray['receiveTime'] . "." . rtrim($rowarray['receiveTime_nsec'],"0");
	$channel = ChannelText($rowarray['channel']);
	$signalStrength_dBm = $rowarray['signalStrength_dBm'];
	$signalToNoiseRatio_dB = $rowarray['signalToNoiseRatio_cB'] / 10; // cB is centiBells
	

	//prints row per entry
	echo
		"<tr class =\"datatable\", height = \"20px\">
		<td class =\"datatable\"> $seqNo </td>
		<td class =\"datatable\"> $receiveTime </td>
		<td class =\"datatable\"> <a href=\"transmissionperformance.php?euiText=$unformattedMoteEui\"> $formattedMoteEui </a> </td>
		<td class =\"datatable\"> $frequency </td>
		<td class =\"datatable\"> $loraModulationText </td>
		<td class =\"datatable\"> $bandwidth_Hz </td>
		<td class =\"datatable\"> $spreadingFactor </td>
		<td class =\"datatable\"> $codingRate </td>
		<td class =\"datatable\"> $loraAdrEnabled </td>
		<td class =\"datatable\"> $channel </td>
		<td class =\"datatable\"> $signalStrength_dBm </td>
		<td class =\"datatable\"> $signalToNoiseRatio_dB </td>
		</tr>";
}

mysqli_free_result($queryResult);

?>

</table>
</tr>
</table> <!-- Data table holder -->
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