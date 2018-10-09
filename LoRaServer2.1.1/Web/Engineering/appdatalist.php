<!DOCTYPE html>
<HTML>

<HEAD>

<title>LoRa&trade; Application data list</title>
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

if(isset($_GET['frameNumberText']))
	$numberOfFrames = RemoveNonNumericCharacters($_GET['frameNumberText'],0); 	//strip non decimal characters from input
else
	$numberOfFrames = MAX_PACKETS_DISPLAYED;

// Query database
//Open connection - then query and close
$connection = SqlConnect(CUSTOMER);

if (!$connection)
{
	echo "Unable to connect to database";
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
</table> <!-- Title table -->

<table> <!-- Data table -->

<!-- Display heading -->
<h2>Application data received from LoRa node 
<?php 
echo $euiFormatted;
?> 
</h2></div>

<?php
$maxPacketsDisplayed = MAX_PACKETS_DISPLAYED;
$query = "SELECT port, time, HEX(seqno) as seqno, data, id FROM appdata WHERE HEX(mote) = \"$moteEui\" ORDER BY id DESC LIMIT $maxPacketsDisplayed";
$queryResult = mysqli_query($connection, $query) or die(mysqli_error($connection));

if ($queryResult == false)
{
	echo "Unable to read database";
	goto endBody;
}

$rowarray = mysqli_fetch_array($queryResult);
?>

<div class = "content">
<br>
<table class = "datatable">
<col class="datatable" width = 40px> <!-- port -->
<col class="datatable" width = 200px> <!-- time -->
<col class="datatable" width = 60px> <!-- seqno -->
<col class="datatable"> <!-- data -->

<thead align = "center">
<tr> 
<th> Port </th>
<th> Time</th>
<th> Sequence #</th>
<th> Application data</th>
</tr>
</thead>

<tbody class="datatable">
<?php
//Display packets (if any)
while($rowarray = mysqli_fetch_array($queryResult))
{
	//prints row per entry
	$port = $rowarray['port'];
	$time = $rowarray['time'];
	$seqno = $rowarray['seqno'];
	$spacedData = chunk_split($rowarray['data'], 2, " ");

	echo "<tr class=\"datatable\" style=\"height = 20px\">
		<td class=\"datatable\"> $port </td>
		<td class=\"datatable\"> $time </td>
		<td class=\"datatable\"> $seqno </td>
		<td class=\"datatable\" style=\"text-align:left\"> $spacedData </td>";

		//<td align=\"left\" valign = \"top\"> ".	FormatByteString($rowarray['data'], 0, ' '). "</td>
	echo "</tr>";	
}

mysqli_free_result($queryResult);
?>
	
</tbody>
</table> <!-- Data table -->
</table>
</table> <!-- Outer table -->
<?php
endBody:
?>
</BODY>
</HTML>