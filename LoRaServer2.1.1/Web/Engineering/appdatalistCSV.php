<?php

ini_set("memory_limit","25M");
include('constants.php');
include('functions.php');

header('Content-type: plain/text');

//Interpret input
if(!isset($_GET['euiText']))
{
	echo "Unable to read mote EUI";
	return;
}

$moteEui = RemoveNonNumericCharacters($_GET['euiText'],1); 	//strip non hex characters from input
$euiFormatted = FormatEui($moteEui ,':');
$filename = "packets-$euiFormatted.csv";

if(isset($_GET['frameNumberText']))
	$numberOfFrames = RemoveNonNumericCharacters($_GET['frameNumberText'],0); 	//strip non decimal characters from input
else
	$numberOfFrames = MAX_PACKETS_DISPLAYED;

// Tell browsers to open up the download dialog box to download the generated file

header("Content-Disposition: attachment; filename=$filename");
// Query database
//Open connection - then query and close
$connection = SqlConnect(CUSTOMER);

if (!$connection)
{
	echo "Unable to connect to database";
	return;
}

$maxPacketsDisplayed = MAX_PACKETS_DISPLAYED;
$query = "SELECT port, time, HEX(seqno) as seqno, data FROM appdata WHERE HEX(mote) = \"$moteEui\" ORDER BY id DESC LIMIT $maxPacketsDisplayed";
$queryResult = mysqli_query($connection, $query) or die(mysqli_error($connection));

if ($queryResult == false)
{
	echo "Unable to read database";
	return;
}

$rowarray = mysqli_fetch_array($queryResult);


echo "Port, Time, Sequence #, Application data\n";

//Display packets (if any)
while($rowarray = mysqli_fetch_array($queryResult))
{
	//prints row per entry
	$port = $rowarray['port'];
	$time = $rowarray['time'];
	$seqno = $rowarray['seqno'];
	$spacedData = chunk_split($rowarray['data'], 2, " ");

	echo "$port, $time, $seqno, $spacedData\n";
}

mysqli_free_result($queryResult);
?>

