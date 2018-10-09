<HTML>

<?php
ini_set("memory_limit","25M");
include('constants.php');
include('functions.php');

?>
<HEAD>

<title>LoRa&trade; demonstrator</title>

<style type="text/css">

h1{
color:#184949;
font-size:30px;
}

</style>

</HEAD>
<BODY>
<DIV class=default>
<table border="0">
<tr>

<td width="70%" valign="top"><h1>LoRa demonstrator</h1> 


<P><A HREF="motelist.php">Motes</A></P>
<P><A HREF="gatewaylist.php">Gateways</A></P>

<P><A HREF="motelastframelist.php">Most recent packet received from each mote</A></P>
<P><A HREF="gpsdemo">GPS demo</A></P>

<?php
nl();
echo "Server version : " . GetServerVersion(NETWORK);
nl();
?>
</TD>
<td width="30%" valign="top"><a href="index.html"><img src="LoRa_square.png" alt="LoRa from Semtech" width="200" height="200"></a></td>
</tr>

</BODY>

</HTML>