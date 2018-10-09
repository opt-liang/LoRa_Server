<?php

include('constants.php');
include('localconstants.php');

function SqlConnect($databaseType)
{
	switch($databaseType)
	{
		case NETWORK:		$databaseName = DB_NAME_NET;	break;
		case APPLICATION:	$databaseName = DB_NAME_APP;	break;
		case CUSTOMER:		$databaseName = DB_NAME_CUST;	break;
		
		default:	return FALSE;
	}
	
	$con = mysqli_connect(DB_HOST, DB_USER, DB_PASSWORD, $databaseName); 
		// hostAddress, username, password, database name
		
	if (!$con)
		return FALSE;
		
	return $con;
}

function RemoveNonNumericCharacters($input, $hex)
{
	if ($hex)
		return preg_replace('/[^0-9a-fA-F]/','',$input);
	else
		return preg_replace('/[^0-9]/','',$input);
}


function AddLeadingZeros($input, $outputLength)
{
	$zeros = $outputLength - strlen($input);
	
	$count = 0;
	$zeroString = "";
	for(; $count < $zeros; $count++)
		$zeroString .= "0";
	
	return $zeroString . $input;
}

function FormatByteString($input, $outputLength, $delimiter)
{
	$correctLengthString = AddLeadingZeros($input, $outputLength);
	
	if (strlen($delimiter) > 0)
		$output = Deliminate($correctLengthString, $delimiter);
	else
		$output = $correctLengthString;
	
	return $output;
}

function FormatNetworkAddress($address, $delimiter = ADDRESS_DELIMITER)
{
	return FormatByteString($address, LORA_SHORT_ADDRESS_CHARS, $delimiter);
}

function FormatEui($address, $delimiter = ADDRESS_DELIMITER)
{
	return FormatByteString($address, LORA_LONG_ADDRESS_CHARS, $delimiter);
}

function Deliminate($number, $delimiter)
{
	$numberArray = str_split($number,2);
	
	for ($i = 0; $i < count($numberArray); $i++)
	{
		if (strlen($numberArray[$i]) < 2)
			$numberArray[$i] = '0' . $numberArray[$i];
	}
	
	$deliminated = implode($delimiter, $numberArray);
	return $deliminated;
}

function NodePacketListLink($unformattedaddress)
{
	$formattedAddress = FormatShortAddress($unformattedaddress);

	$nodePacketListLink = "<a href=\"packetlistbynode.php?nodeAddressText=" . $unformattedaddress . "&submit=List\">" . $formattedAddress . "</a>";
	
	return $nodePacketListLink;
}

function GatewayPacketListLink($unformattedaddress)
{
	$formattedAddress = FormatEui($unformattedaddress);

	$gatewayPacketListLink = "<a href=\"packetlistbygateway.php?gatewayAddressText=" . $unformattedaddress . "&submit=List\">" . $formattedAddress . "</a>";

	return $gatewayPacketListLink;
}

function nl()
{
	echo '<br>';
}

function message($string)
{
	echo "<script type='text/javascript'>";
	echo "alert('$string');";
	echo "</script>"; 
}
function redirect($path)
{
	echo "<script type='text/javascript'>";
	echo "location.href = '$path';";
	echo "</script>";
}

function ChannelText($channelNo)
{
	if ($channelNo < 7)
		$result = "LC" . ($channelNo + 1);
	else if ($channelNo == 7)
		$result = "FC1";
	else
		$result = "ERR";
		
	return $result;
}

function TimeText($receiveTime, $receiveTime_nsec = 0, $receiveTimeAccurate = false)
{
	$result = $receiveTime;
	
	if ($receiveTime_nsec != 0 || $receiveTimeAccurate)
	{
		//Remove trailing zeros
		$receiveTime_nsec = rtrim($receiveTime_nsec,"0");
		
		if (empty($receiveTime_nsec) && $receiveTimeAccurate)
			$receiveTime_nsec = "0";
		
		$result .= ".$receiveTime_nsec";
	}
	return $result;
}


function SetMoteLED($moteEui, $state)	// state is "on" or "off"
{
	$appEui = "0";
	$loraport = "2";

	if ($state == "on")
		$msgData = "\x01";
	else
		$msgData = "\x00";
	$msgBase64 = base64_encode($msgData);

	$jsoncommand = "{\"app\":{\"data\":{\"source\":\"user\",\"appeui\":\"$appEui\",\"moteeui\":\"$moteEui\",\"port\":$loraport,\"msg\":\"$msgBase64\",\"ackrq\":\"false\"}}}";
	
	
	$destination = "127.0.0.1";
	$udpPort = ReadConfiguration("appServerJsonSocket", APPLICATION);
	
	SendUpdPacket($destination, $udpPort, $jsoncommand);
}


function SendUpdPacket($destination, $port, $data)
{
	if (!$socket = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP))
		return false;
	
	socket_sendto($socket, $data, strlen($data), 0, $destination, $port);
	return true;
}

function ReadConfiguration($name, $database)
{
	$query = "SELECT value FROM configuration WHERE name = \"$name\"";
	
	$connection = SqlConnect($database);

	$result = "";
	if (!$connection)
	{
		echo "Unable to connect to database";
		return $result;
	}	
	$queryResult = mysqli_query($connection, $query) or die(mysqli_error($connection));
	mysqli_close($connection);
	
	if($rowarray = mysqli_fetch_array($queryResult))
		$result = $rowarray['value'];

	mysqli_free_result($queryResult);

	return $result;
}

function GetServerVersion($serverDatabase)
{
	$buildDate = ReadConfiguration("buildDate", $serverDatabase);
	$buildTime = ReadConfiguration("buildTime", $serverDatabase);
	$buildVersion = ReadConfiguration("buildVersion", $serverDatabase);
	
	return "$buildVersion - Build date $buildDate $buildTime GMT";
}


function ReadTime($inputText)
{
	if (substr($inputText, 0, 1) === '-')
	{
		$negative = true;
	}
	else
	{
		$negative = false;
	}
	
	$scanResult = sscanf($inputText,"%d:%d:%d");
	
	$days = (int)($scanResult[0] / HOURS_IN_DAY);
	$hours = (int)($scanResult[0] - $days * HOURS_IN_DAY);
	
	$result = array(
		'negative' => $negative,
		'days' => $days,
		'hours' => $hours,
		'minutes' => $scanResult[1],
		'seconds' => $scanResult[2]
	);
	
	return $result;
}

function WriteTime($timeArray)
{
	$timeText = "";
	
	if ($timeArray['negative'])
		$timeText .= "-";
	if ($timeArray['days'] > 0)
		$timeText .= $timeArray['days'] . " days ";
	$timeText .= sprintf("%02u:%02u:%02u",$timeArray['hours'], $timeArray['minutes'], $timeArray['seconds']);
	
	return $timeText;
}

function IntegerSeconds($timeArray)
{
	$result = HOURS_IN_DAY * $timeArray['days'] + $timeArray['hours'];
	$result = $result * MINS_IN_HOUR + $timeArray['minutes'];
	$result = $result * SECS_IN_HOUR + $timeArray['seconds'];
	
	if ($timeArray['negative'] != 0)
		$result = -$result;

	return $result;
}


function FindColour($integerSeconds)
{
	if ($integerSeconds < SECS_TO_WARN)
	{
		$colour = GOOD;	
	}
	else if ($integerSeconds < SECS_TO_FAIL)	
	{
		$colour = WARN;
	}
	else
	{
		$colour = FAIL;
	}
	return $colour;
}

function ConvertBase64ToHex($input, $space = false)
{
	$binary = base64_decode($input);
	
	$unspacedOutput = bin2hex($binary);
	
	if (!$space)
		return $unspacedOutput;
	
	$spacedOut = chunk_split($unspacedOutput, 2, " ");
	
	return $spacedOut;
}

function ConvertHexToBase64($input)
{
	$input = RemoveNonNumericCharacters($input, true);
	
	$binary = "";
	foreach (str_split($input, 2) as $pair)
	{
		$binary .= chr(hexdec($pair));
	}
	
	$output = base64_encode($binary);
	
	return $output;
}
?>

