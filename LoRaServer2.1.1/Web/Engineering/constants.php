<?php

if (!defined('CONSTANTS'))
{
	define('CONSTANTS',TRUE);
	define('LORA_SHORT_ADDRESS_BYTES', 4);
	define('LORA_SHORT_ADDRESS_CHARS', LORA_SHORT_ADDRESS_BYTES * 2);
	define('LORA_LONG_ADDRESS_BYTES', 8);
	define('LORA_LONG_ADDRESS_CHARS', LORA_LONG_ADDRESS_BYTES * 2);
	define('ADDRESS_DELIMITER', ':');
	define('SQL_BASE', 10);
	define('HEX', 16);
	define('MAX_PACKETS_DISPLAYED', 200);
	define('SECS_IN_MIN', 60);
	define('MINS_IN_HOUR', 60);
	define('SECS_IN_HOUR', SECS_IN_MIN * MINS_IN_HOUR);
	define('HOURS_IN_DAY', 24);
	define('SECS_IN_DAY', SECS_IN_HOUR * HOURS_IN_DAY);
	
	define('SECS_TO_WARN', 1 * SECS_IN_MIN);
	define('SECS_TO_FAIL', 5 * SECS_IN_MIN);
	
	define('GOOD','#000000');
	define('WARN','#FFAA00');
	define('FAIL','#FF0000');

	define('ACCEPT_NUMBER', 3);
	define('ACCEPT_UNIT', "day"); //word is always singular
	define('ACCEPT_MARGIN', ACCEPT_NUMBER . " " . ACCEPT_UNIT);
	
	define('COUNT_NUMBER', 1);
	define('COUNT_UNIT', "hour"); //word is always singular
	define('COUNT_MARGIN', COUNT_NUMBER . " " . COUNT_UNIT);
	
	define('LOOPBACK_ADDRESS', "127.0.0.1");
	
	define('NETWORK',0);
	define('APPLICATION',1);
	define('CUSTOMER', 2);
}
?>