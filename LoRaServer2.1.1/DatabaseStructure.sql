-- phpMyAdmin SQL Dump
-- version 4.2.11
-- http://www.phpmyadmin.net
--
-- Host: 127.0.0.1
-- Generation Time: Jul 10, 2015 at 10:08 AM
-- Server version: 5.6.21
-- PHP Version: 5.6.3
/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: 'lora_application'
--
CREATE DATABASE IF NOT EXISTS lora_application DEFAULT CHARACTER SET armscii8 COLLATE armscii8_general_ci;
USE lora_application;

-- --------------------------------------------------------

--
-- Table structure for table 'activemotes'
--

CREATE TABLE IF NOT EXISTS activemotes (
  eui bigint(20) unsigned NOT NULL,
  appEui bigint(20) unsigned NOT NULL,
  sessionKey varchar(32) NOT NULL,
  networkAddress int(10) unsigned NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'applications'
--

CREATE TABLE IF NOT EXISTS applications (
  eui bigint(20) unsigned NOT NULL,
  `name` varchar(100) NOT NULL,
  `owner` varchar(100) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'configuration'
--

CREATE TABLE IF NOT EXISTS configuration (
  `name` varchar(50) NOT NULL,
  `value` varchar(50) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'joinmotes'
--

CREATE TABLE IF NOT EXISTS joinmotes (
  eui bigint(20) unsigned NOT NULL,
  appEui bigint(20) unsigned NOT NULL,
  appKey varchar(32) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'nonces'
--

CREATE TABLE IF NOT EXISTS nonces (
  mote bigint(20) unsigned NOT NULL,
  nonce smallint(5) unsigned NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'servers'
--

CREATE TABLE IF NOT EXISTS servers (
  appEui bigint(20) unsigned NOT NULL,
  addressText varchar(30) NOT NULL COMMENT 'Because ''''.'''' is a special character in SQL searches, it is replaced in this field by ''''@''''',
  nullApplication tinyint(1) NOT NULL DEFAULT '0',
  active tinyint(1) NOT NULL COMMENT 'active connections attempt to create a TCP connection to the remote port',
  serviceText varchar(100) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

--
-- Indexes for dumped tables
--

--
-- Indexes for table activemotes
--
ALTER TABLE activemotes
 ADD PRIMARY KEY (eui);

--
-- Indexes for table applications
--
ALTER TABLE applications
 ADD PRIMARY KEY (eui), ADD UNIQUE KEY nameOwner (`name`,`owner`);

--
-- Indexes for table configuration
--
ALTER TABLE configuration
 ADD PRIMARY KEY (`name`);

--
-- Indexes for table joinmotes
--
ALTER TABLE joinmotes
 ADD PRIMARY KEY (eui);

--
-- Indexes for table nonces
--
ALTER TABLE nonces
 ADD PRIMARY KEY (mote,nonce);

--
-- Indexes for table servers
--
ALTER TABLE servers
 ADD PRIMARY KEY (appEui,addressText,nullApplication);
--
-- Database: 'lora_customer'
--
CREATE DATABASE IF NOT EXISTS lora_customer DEFAULT CHARACTER SET armscii8 COLLATE armscii8_general_ci;
USE lora_customer;

-- --------------------------------------------------------

--
-- Table structure for table 'appdata'
--

CREATE TABLE IF NOT EXISTS appdata (
id bigint(20) unsigned NOT NULL,
  mote bigint(20) unsigned NOT NULL,
  `time` timestamp NULL DEFAULT NULL,
  time_usec mediumint(8) unsigned DEFAULT NULL,
  accurateTime tinyint(1) DEFAULT NULL,
  seqNo int(10) unsigned NOT NULL,
  `port` tinyint(3) unsigned NOT NULL,
  `data` varchar(500) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'applications'
--

CREATE TABLE IF NOT EXISTS applications (
  eui bigint(20) unsigned NOT NULL,
  `name` varchar(100) NOT NULL,
  `owner` varchar(100) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'configuration'
--

CREATE TABLE IF NOT EXISTS configuration (
  `name` varchar(50) NOT NULL,
  `value` varchar(50) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'gatewayframerx'
--

CREATE TABLE IF NOT EXISTS gatewayframerx (
  id bigint(20) unsigned NOT NULL COMMENT 'reference to appdata.id',
  rank smallint(5) unsigned NOT NULL,
  gatewayEui bigint(20) unsigned NOT NULL,
  receiveTime timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  receiveTime_nsec int(10) unsigned DEFAULT NULL COMMENT 'nanosecond part of receive time',
  receiveTimeAccurate tinyint(1) NOT NULL,
  channel tinyint(3) unsigned DEFAULT NULL,
  rfChain tinyint(3) unsigned DEFAULT NULL,
  signalToNoiseRatio_cB mediumint(9) DEFAULT NULL COMMENT 'Unit is centiBells (10 x a dB)',
  signalStrength_dBm smallint(6) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'gateways'
--

CREATE TABLE IF NOT EXISTS gateways (
  eui bigint(20) unsigned NOT NULL,
  lastRxFrame bigint(20) unsigned NOT NULL COMMENT 'id of most recently received frame'
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'moteframetx'
--

CREATE TABLE IF NOT EXISTS moteframetx (
  id bigint(20) unsigned NOT NULL COMMENT 'reference to appdata.id',
  frequency int(10) unsigned NOT NULL,
  loraModulation tinyint(1) NOT NULL COMMENT 'false represents FSK',
  loraModulationBandwidth_Hz int(10) unsigned DEFAULT NULL,
  loraSpreadingFactor tinyint(2) unsigned DEFAULT NULL,
  loraCodingRateNumerator smallint(3) unsigned DEFAULT NULL,
  loraCodingRateDenominator smallint(3) unsigned DEFAULT NULL,
  loraAdrEnabled tinyint(1) DEFAULT NULL,
  fskBitRate int(10) unsigned DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'motes'
--

CREATE TABLE IF NOT EXISTS motes (
  eui bigint(20) unsigned NOT NULL,
  appEui bigint(20) unsigned NOT NULL,
  lastRxFrame bigint(20) unsigned NOT NULL COMMENT 'id of most recently received frame'
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'servers'
--

CREATE TABLE IF NOT EXISTS servers (
  appEui bigint(20) unsigned NOT NULL,
  addressText varchar(30) NOT NULL COMMENT 'Because ''''.'''' is a special character in SQL searches, it is replaced in this field by ''''@''''',
  nullApplication tinyint(1) NOT NULL DEFAULT '0',
  active tinyint(1) NOT NULL COMMENT 'active connections attempt to create a TCP connection to the remote port',
  serviceText varchar(100) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

--
-- Indexes for dumped tables
--

--
-- Indexes for table appdata
--
ALTER TABLE appdata
 ADD PRIMARY KEY (id);

--
-- Indexes for table applications
--
ALTER TABLE applications
 ADD PRIMARY KEY (eui), ADD UNIQUE KEY nameOwner (`name`,`owner`);

--
-- Indexes for table configuration
--
ALTER TABLE configuration
 ADD PRIMARY KEY (`name`);

--
-- Indexes for table gatewayframerx
--
ALTER TABLE gatewayframerx
 ADD PRIMARY KEY (id,gatewayEui), ADD KEY id (id);

--
-- Indexes for table gateways
--
ALTER TABLE gateways
 ADD PRIMARY KEY (eui);

--
-- Indexes for table moteframetx
--
ALTER TABLE moteframetx
 ADD PRIMARY KEY (id);

--
-- Indexes for table motes
--
ALTER TABLE motes
 ADD PRIMARY KEY (eui);

--
-- Indexes for table servers
--
ALTER TABLE servers
 ADD PRIMARY KEY (appEui,addressText,nullApplication);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table appdata
--
ALTER TABLE appdata
MODIFY id bigint(20) unsigned NOT NULL AUTO_INCREMENT;--
-- Database: 'lora_network'
--
CREATE DATABASE IF NOT EXISTS lora_network DEFAULT CHARACTER SET armscii8 COLLATE armscii8_general_ci;
USE lora_network;

-- --------------------------------------------------------

--
-- Table structure for table 'applications'
--

CREATE TABLE IF NOT EXISTS applications (
  eui bigint(20) unsigned NOT NULL,
  `name` varchar(100) NOT NULL,
  `owner` varchar(100) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'configuration'
--

CREATE TABLE IF NOT EXISTS configuration (
  `name` varchar(50) NOT NULL,
  `value` varchar(50) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'gateways'
--

CREATE TABLE IF NOT EXISTS gateways (
  eui bigint(20) unsigned NOT NULL,
  region tinyint(3) unsigned DEFAULT NULL COMMENT 'Enumerated type 0=americas902, 1=china779, 2=europe433, 3=europe863',
  allowGpsToSetPosition tinyint(1) NOT NULL DEFAULT '1',
  `time` timestamp NULL DEFAULT NULL,
  latitude double DEFAULT NULL,
  longitude double DEFAULT NULL,
  altitude double DEFAULT NULL,
  uppacketsreceived int(10) unsigned DEFAULT '0',
  gooduppacketsreceived int(10) unsigned NOT NULL DEFAULT '0',
  uppacketsforwarded int(10) unsigned NOT NULL DEFAULT '0',
  uppacketsacknowedgedratio float NOT NULL DEFAULT '0',
  downpacketsreceived int(10) unsigned NOT NULL DEFAULT '0',
  packetstransmitted int(10) unsigned NOT NULL DEFAULT '0',
  lastuppacketid bigint(20) unsigned DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=armscii8 ROW_FORMAT=FIXED;

-- --------------------------------------------------------

--
-- Table structure for table 'motes'
--

CREATE TABLE IF NOT EXISTS motes (
  eui bigint(20) unsigned NOT NULL DEFAULT '0',
  appeui bigint(20) unsigned NOT NULL,
  networkAddress int(10) unsigned NOT NULL,
  networkSessionKey varchar(32) NOT NULL,
  downMsgSeqNo int(20) unsigned NOT NULL DEFAULT '0' COMMENT 'Only more significant bits are written (number is implementation dependant)',
  upMsgSeqNo int(20) unsigned NOT NULL DEFAULT '0' COMMENT 'Only more significant bits are written (number is implementation dependant)'
) ENGINE=MyISAM DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'servers'
--

CREATE TABLE IF NOT EXISTS servers (
  appEui bigint(20) unsigned NOT NULL,
  addressText varchar(30) NOT NULL COMMENT 'Because ''''.'''' is a special character in SQL searches, it is replaced in this field by ''''@''''',
  nullApplication tinyint(1) NOT NULL DEFAULT '0',
  active tinyint(1) NOT NULL COMMENT 'active connections attempt to create a TCP connection to the remote port',
  serviceText varchar(100) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

--
-- Indexes for dumped tables
--

--
-- Indexes for table applications
--
ALTER TABLE applications
 ADD PRIMARY KEY (eui), ADD UNIQUE KEY nameOwner (`name`,`owner`);

--
-- Indexes for table configuration
--
ALTER TABLE configuration
 ADD PRIMARY KEY (`name`);

--
-- Indexes for table gateways
--
ALTER TABLE gateways
 ADD PRIMARY KEY (eui);

--
-- Indexes for table motes
--
ALTER TABLE motes
 ADD PRIMARY KEY (eui);

--
-- Indexes for table servers
--
ALTER TABLE servers
 ADD PRIMARY KEY (appEui,addressText,nullApplication);
--
-- Database: 'lora_networkcontroller'
--
CREATE DATABASE IF NOT EXISTS lora_networkcontroller DEFAULT CHARACTER SET ascii COLLATE ascii_general_ci;
USE lora_networkcontroller;

-- --------------------------------------------------------

--
-- Table structure for table 'applications'
--

CREATE TABLE IF NOT EXISTS applications (
  eui bigint(20) unsigned NOT NULL,
  `name` varchar(100) NOT NULL,
  `owner` varchar(100) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'configuration'
--

CREATE TABLE IF NOT EXISTS configuration (
  `name` varchar(50) NOT NULL,
  `value` varchar(50) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'motes'
--

CREATE TABLE IF NOT EXISTS motes (
  eui bigint(20) unsigned NOT NULL,
  appEui bigint(20) unsigned NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

-- --------------------------------------------------------

--
-- Table structure for table 'servers'
--

CREATE TABLE IF NOT EXISTS servers (
  appEui bigint(20) unsigned NOT NULL,
  addressText varchar(30) NOT NULL COMMENT 'Because ''.'' is a special character in SQL searches, it is replaced in this field by ''@''',
  nullApplication tinyint(1) NOT NULL DEFAULT '0',
  active tinyint(1) NOT NULL COMMENT 'active connections attempt to create a TCP connection to the remote port',
  serviceText varchar(100) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=armscii8;

--
-- Indexes for dumped tables
--

--
-- Indexes for table applications
--
ALTER TABLE applications
 ADD PRIMARY KEY (eui), ADD UNIQUE KEY nameOwner (`name`,`owner`);

--
-- Indexes for table configuration
--
ALTER TABLE configuration
 ADD PRIMARY KEY (`name`);

--
-- Indexes for table motes
--
ALTER TABLE motes
 ADD PRIMARY KEY (eui);

--
-- Indexes for table servers
--
ALTER TABLE servers
 ADD PRIMARY KEY (appEui,addressText);

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
