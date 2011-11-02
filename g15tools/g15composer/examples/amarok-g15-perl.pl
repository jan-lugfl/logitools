#!/usr/bin/perl -w

use strict;
use DCOP::Amarok::Player;
use threads;
use threads::shared;

my $user = "$ENV{USER}";
my $player = DCOP::Amarok::Player->new( user => $user ) or die "Couldn't Attach DCOP Interface: $!\n";
my $controlPipe = "/var/run/g15composer";
my $pipe = "$ENV{HOME}/.g15amaroklcdpipe";

my $vol : shared = $player->getVolume;
my $status  :  shared = ( $player->status() > 1 ) ? 1 : 0;

$SIG{TERM} = \&bye;

open(CPIPE, ">>$controlPipe");
print CPIPE "SN \"$pipe\"\n";
close(CPIPE);
sleep 1;

open(PIPE, ">>$pipe");
print PIPE "FL 0 10 \"/usr/share/fonts/ttf-bitstream-vera/Vera.ttf\"\n";

sub volUpdate {
	print PIPE "PC 0\n";
	print PIPE "TO 0 10 2 1 \"Volume\"\n";
	print PIPE "DB 3 27 157 35 2 $vol 100 3\n";
}

&stoppedScreen();

while($status == 0) {
	$status = ( $player->status() > 1 ) ? 1 : 0;
	sleep 1;
}

my $artist : shared = $player->artist;
my $title : shared = $player->title;
my $album : shared = $player->album;
my $trackTotalSecs : shared = $player->totaltimesecs;
my $trackCurSecs : shared = $player->trackCurrentTime;
my $trackTotalTime : shared = $player->totalTime;
my $trackCurTime : shared = $player->currentTime;

&initScreen();

my $progressThread = threads->create(\&progress);
$progressThread->detach();

while(1) {
	$_ = <STDIN>;
	chomp $_;
	if ( /trackChange/ ) {
		$artist = $player->artist;
		$title = $player->title;
		$album = $player->album;
		$trackTotalSecs = $player->totaltimesecs;
		$trackTotalTime = $player->totalTime;
		$trackCurSecs = $player->trackCurrentTime;
		$trackCurTime = $player->currentTime;
		initScreen();
		$artist = $player->artist;
		$title = $player->title;
		$album = $player->album;
		$trackTotalSecs = $player->totaltimesecs;
		$trackTotalTime = $player->totalTime;
		$trackCurSecs = $player->trackCurrentTime;
		$trackCurTime = $player->currentTime;
		initScreen();
		bringToFront();
	} elsif ( /engineStateChange/ ) {
                if( /playing/ ) {
                        $status = 1;
			initScreen();
                } elsif ( /pause/ ) {
                        $status = 2;
                } else {
                        $status = 0;
			stoppedScreen();
			stoppedScreen();
                }
        } elsif( /volumeChange/ ){
                m/: (\d+)/;
                $vol = $1;
		$status = 0;
		&volUpdate();
		$status = 1;
        }
	exit if( /exit/ );
	&bye() if( /kill/ );
}

sub bringToFront {
	print PIPE "MP 0\n";
	sleep 3;
	print PIPE "MP 2\n";
}

sub stoppedScreen {
	print PIPE "MC 0\n";
	print PIPE "PC 0\n";
	print PIPE "DR 0 0 159 43 1 1\n";
	print PIPE "DR 3 22 157 40 0 1\n";
	print PIPE "PB 3 22 157 24 0\n";
	print PIPE "FP 0 15 0 10 0 1 \"Playback Stopped\"\n";
	print PIPE "MC 1\n";
}

sub initScreen {
	print PIPE "PC 0\n";
	print PIPE "DR 0 0 159 43 1 1\n";
	print PIPE "DR 3 22 157 40 0 1\n";
	print PIPE "PB 3 22 157 24 0\n";
	print PIPE "MX 1\n";
	print PIPE "FP 0 15 0 0 1 1 \"$artist\"\n";
	print PIPE "FP 0 8 0 13 1 1 \"$album\"\n";
	print PIPE "FP 0 7 0 23 1 1 \"$title\"\n";
	print PIPE "DB 5 30 155 38 1 $trackCurSecs $trackTotalSecs 1\n";
	print PIPE "TO 0 32 0 1 \"$trackCurTime / $trackTotalTime\"\n";
	print PIPE "MX 0\n";
	print PIPE "DL 5 29 5 39 1\n";
	print PIPE "PS 5 29 0\n";
	print PIPE "PS 5 39 0\n";
	print PIPE "MC 1\n";
}

sub progress {
	while(1) {
		if($status == 1) {
			$trackCurSecs = $player->trackCurrentTime;
			$trackCurTime = $player->currentTime;
			print PIPE "PB 5 29 155 40 0 1 1\n";
			print PIPE "MX 1\n";
			print PIPE "DB 5 30 155 38 1 $trackCurSecs $trackTotalSecs 1\n";
			print PIPE "TO 0 32 0 1 \"$trackCurTime / $trackTotalTime\"\n";
			print PIPE "MX 0\n";
			print PIPE "PB 5 29 5 39 1\n";
			print PIPE "PS 5 29 0\n";
			print PIPE "PS 5 39 0\n";
			print PIPE "MC 1\n";
		} elsif($status == 0) {
			stoppedScreen();
			$status = 2;
		} elsif($status == -1) {
			return;
		}
		sleep 1;
	}
}
	
sub bye {
	$status = -1;
	sleep 2;
	print PIPE "SC\n";
	close(PIPE);
	exit 0;
}
