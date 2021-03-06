#!/usr/bin/perl

use strict;
use CGI qw(:standard);
use kjpbs_captcha;

our @services;
require "kjpbs_nodes.pl";

my $captcha = kjpbs_captcha->new;

my $challenge = cookie('hash');
my $response = param 'verify';
my $resolution = param 'resolution';
my $bbl = param 'bbl';
my $bible = $bbl;

# Limit uncontrolled input:
if ($resolution eq '1024x768') {
} elsif ($resolution eq '1280x720') {
} elsif ($resolution eq '1280x1024') {
} elsif ($resolution eq '1440x810') {
} elsif ($resolution eq '1920x1080') {
} else {
  $resolution = '1280x1024';
}

# Limit uncontrolled input and remap Bible indexes accordingly:
if ($bbl eq '1') {
  $bible = '1';
} elsif ($bbl eq '2') {
  $bible = '12';
} elsif ($bbl eq '3') {
  $bible = '21';
} elsif ($bbl eq '4') {
  $bible = '16';
} elsif ($bbl eq '5') {
  $bible = '7';
} else {
  $bible = '1';
}

# Verify Submission
my $result = $captcha->captcha_verify($challenge, $response);

my $nodeID;
my $launchURL;
my $datetimestring;

if ($result) {
  $nodeID = launch_node($resolution, $bible, '60m');

  if ($nodeID > -1) {
    $launchURL = "http://vnc.purebiblesearch.com:" . $services[$nodeID][3];
    $launchURL .= "/vnc.html?host=vnc.purebiblesearch.com&port=" . $services[$nodeID][3];
    $launchURL .= "&true_color=1&autoconnect=1&cursor=0";

    sleep 5;
    print redirect($launchURL);
  } else {
    open (MYFILE, '>>/var/www/data/kjpbs/server_full.txt');
    $datetimestring = gmtime();
    print MYFILE "$datetimestring\n";
    close (MYFILE);
    print header;
    print start_html('King James Pure Bible Search VNC Service');
    print "<p>Sorry, all King James Pure Bible Search VNC Sessions are in use.</p>\n<br><br>\n";
    print "<a href=\"http://vnc.purebiblesearch.com/status/\">Click here to see status</a><br>\n";
    print end_html;
  }
} else {
  print redirect("http://vnc.purebiblesearch.com/?resolution=$resolution&bbl=$bbl");
}

exit;
