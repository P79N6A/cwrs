<?php
if (!defined('FREEPBX_IS_AUTH')) { die('No direct script access allowed'); }
//This file is part of FreePBX.
//
//    FreePBX is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 2 of the License, or
//    (at your option) any later version.
//
//    FreePBX is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with FreePBX.  If not, see <http://www.gnu.org/licenses/>.
//
//    cdr module for FreePBX 2.7+
//    Copyright (C) Mikael Carlsson
//
// Update cdr database with did field
//
global $db;
global $amp_conf;
// Retrieve database and table name if defined, otherwise use FreePBX default
$db_name = !empty($amp_conf['CDRDBNAME'])?$amp_conf['CDRDBNAME']:"asteriskcdrdb";
$db_table_name = !empty($amp_conf['CDRDBTABLENAME'])?$amp_conf['CDRDBTABLENAME']:"cdr";
if (! function_exists("out")) {
        function out($text) {
                echo $text."<br />";
        }
}
out(_("Checking if field did is present in cdr table.."));
$sql = "SELECT did FROM $db_name.$db_table_name";
$confs = $db->getRow($sql, DB_FETCHMODE_ASSOC);
if (DB::IsError($confs)) { // no error... Already there
  out(_("Adding did field to cdr"));
  out(_("This might take a while......"));
  $sql = "ALTER TABLE $db_name.$db_table_name ADD did VARCHAR ( 50 ) NOT NULL DEFAULT ''";
  $results = $db->query($sql);
  if(DB::IsError($results)) {
    die($results->getMessage());
  }
  out(_("Added field did to cdr"));
} else {
  out(_("did field already present."));
}

out(_("Checking if field recordingfile is present in cdr table.."));
$sql = "SELECT recordingfile FROM $db_name.$db_table_name";
$confs = $db->getRow($sql, DB_FETCHMODE_ASSOC);
if (DB::IsError($confs)) { // no error... Already there
    out(_("Adding recordingfile field to cdr"));
    $sql = "ALTER TABLE $db_name.$db_table_name ADD recordingfile VARCHAR ( 255 ) NOT NULL DEFAULT ''";
    $results = $db->query($sql);
    if(DB::IsError($results)) {
        out(_('Unable to add recordingfile field to cdr table'));
        freepbx_log(FPBX_LOG_ERROR,"failed to add recordingfile field to cdr table");
    } else {
        out(_("Added field recordingfile to cdr"));
    }
} else {
      out(_("recordingfile field already present."));
}
