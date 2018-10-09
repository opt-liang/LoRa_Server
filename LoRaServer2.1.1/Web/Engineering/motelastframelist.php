<!DOCTYPE html>
<HTML>

<HEAD>

<title>Most recent packet rx from each LoRa node</title>
<link rel="stylesheet" href="lorastyle.css">
</HEAD>
<BODY>

<?php
ini_set("memory_limit","25M");
include('constants.php');
include('localconstants.php');
include('functions.php');



// Query database
//Open connection - then query and close
$connection = SqlConnect(CUSTOMER);

if (!$connection)
{
	echo "Unable to connect to database";
	goto endBody;
}

$query = "SELECT a.id, HEX(m.eui) as mote, HEX(a.seqNo) as seqNo, tx.frequency, tx.loraModulation, tx.modulationBandwidth_Hz, tx.loraCodingRateNumerator, tx.loraCodingRateDenominator, tx.loraAdrEnabled, tx.loraSpreadingFactor, tx.fskBitRate, HEX(rx.gatewayEui) AS gatewayEui, rx.receiveTime, rx.receiveTime_nsec, rx.receiveTimeAccurate, rx.channel, rx.signalStrength_dBm, rx.signalToNoiseRatio_cB FROM activemotes AS m JOIN moteframetx AS tx ON m.lastRxFrame = tx.id JOIN appdata AS a ON a.id = m.lastRxFrame JOIN gatewayframerx AS rx ON rx.id = m.lastRxFrame ORDER BY mote ASC, rx.rank ASC";

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
<h2>List of frame most recently received by each LoRa mote</h2>


<table class="datatable"><!-- Data table -->
<col class="datatable" style="width:120px"> <!-- Mote EUI -->
<col class="datatable" style="width:40px"> <!-- Seq no -->
<col class="datatable" style="width:40px"> <!-- Frequency -->
<col class="datatable" style="width:40px"> <!-- Modulation -->
<col class="datatable" style="width:40px"> <!-- Modulation bandwidth -->
<col class="datatable" style="width:40px"> <!-- Spreading factor -->
<col class="datatable" style="width:40px"> <!-- Coding rate -->
<col class="datatable" style="width:40px"> <!-- ADR enabled -->
<col class="datatable" style="width:160px"> <!-- Gateway EUI -->
<col class="datatable" style="width:220px"> <!-- Receive time -->
<col class="datatable" style="width:40px"> <!-- Channel -->
<col class="datatable" style="width:40px"> <!-- Signal strength -->
<col class="datatable" style="width:40px"> <!-- Signal to noise ratio -->

<tr class="datatable">
<th> Mote EUI </th>
<th> Sequence # </th>
<th> Frequency (MHz)</th>
<th> Modulation </th>
<th> LoRa Modulation bandwidth (Hz) or FSK bit rate (bit/s)</th>
<th> Spreading factor </th>
<th> Coding rate </th>
<th> Adaptive data rate </th>
<th> Gateway EUI </th>
<th> Receive time </th>
<th> Channel </th>
<th> Signal strength (dBm) </th>
<th> Signal to noise ratio (dB) </th>
</tr>

<?php
$oldId = "first";//illegal value
	
$receiveCount = 0;
$singleRowOutput = "";
$multiRowOutput = "";
//Display motes (if any)
while ($rowarray = mysqli_fetch_array($queryResult))
{
	//one frame may be transmitted once but received many times
	$id = $rowarray['id'];
	if ($id != $oldId)
	{
		//new frame
		if ($oldId != "first")
		{
			//print line
			echo str_replace("XXX",$receiveCount,$singleRowOutput);
			echo "$multiRowOutput </tr>";
		}
		$oldId = $id;
		$receiveCount = 0;
		
		$moteEui = $rowarray['mote'];
		$formattedMoteEui = FormatEui($moteEui);
		$seqNo = $rowarray['seqNo'];
		$frequency = $rowarray['frequency'] / 1e6;
		$loraModulation = $rowarray['loraModulation'];
		$modulation = $loraModulation ? "LoRa" : "FSK";
		
		if ($loraModulation)
		{
			$modulationBandwidth_Hz = $rowarray['modulationBandwidth_Hz'] . "Hz";
			$loraSpeadingFactor = "SF" . $rowarray['loraSpreadingFactor'];
			$loraCodingRateNumerator = $rowarray['loraCodingRateNumerator'];
			$loraCodingRateDenominator = $rowarray['loraCodingRateDenominator'];
			$loraCodingRate = $loraCodingRateNumerator . "/" . $loraCodingRateDenominator;
			$loraAdrEnabled = $rowarray['loraAdrEnabled'] ? "on" : "off";
		}
		else
		{
			$modulationBandwidth_Hz = $rowarray['fskBitRate']. "bit/s";
			$loraSpeadingFactor = "";
			$loraCodingRate = "";
			$loraAdrEnabled = "";
		}

		$singleRowOutput = "<tr class=\"datatable\" rowspan=\"XXX\">
			<td rowspan=\"XXX\"> <a href=\"mote.php?euiText=$moteEui\"> $formattedMoteEui</a> </td>
			<td rowspan=\"XXX\"> $seqNo </td>
			<td rowspan=\"XXX\"> $frequency </td>
			<td rowspan=\"XXX\"> $modulation </td>
			<td rowspan=\"XXX\"> $modulationBandwidth_Hz </td>
			<td rowspan=\"XXX\"> $loraSpeadingFactor </td>
			<td rowspan=\"XXX\"> $loraCodingRate </td>
			<td rowspan=\"XXX\"> $loraAdrEnabled </td>
			";
		$multiRowOutput = "";
	}
	else
		$multiRowOutput .= "<tr class=\"datatable\">";
	
	$gatewayEui = $rowarray['gatewayEui'];
	$formattedGatewayEui = FormatEui($gatewayEui);
	$receiveTime = $rowarray['receiveTime'];

	$receiveTime_nsec = $rowarray['receiveTime_nsec'];
	$receiveTimeAccurate = $rowarray['receiveTimeAccurate'];
	
	$receiveTime = TimeText($receiveTime, $receiveTime_nsec, $receiveTimeAccurate);	
	$channel = ChannelText($rowarray['channel']);
	$signalStrength_dBm = $rowarray['signalStrength_dBm'];
	$signalToNoiseRatio_dB = $rowarray['signalToNoiseRatio_cB'] / 10; // cB is centiBells
	
	$multiRowOutput .= "
		<td> <a href=\"framelistbygateway.php?euiText=$gatewayEui\">$formattedGatewayEui</a> </td>
		<td style=\"text-align:left;padding-left:10px;\"> $receiveTime </td>
		<td> $channel </td>
		<td> $signalStrength_dBm </td>
		<td> $signalToNoiseRatio_dB </td> </tr>";
	
	$receiveCount++;
}

if (!empty($singleRowOutput))
{
	echo str_replace("XXX",$receiveCount,$singleRowOutput);
	echo "$multiRowOutput </tr>";
}
mysqli_free_result($queryResult);

?>
</table><!-- Data table-->
</tr> <!-- Data table holder -->
</table> <!-- Data table holder -->
</tr>
</table><!-- Outer table -->
<?php
endBody:
?>
</BODY>

</HTML>