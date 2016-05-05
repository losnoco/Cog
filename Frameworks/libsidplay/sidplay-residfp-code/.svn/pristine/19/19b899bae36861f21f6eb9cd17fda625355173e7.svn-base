#!/usr/bin/perl

#use strict;

use Fcntl;
use Digest::MD5;

my $file_name = $ARGV[0];

sysopen my $bin, $file_name, O_RDONLY | O_BINARY or die "can't open file: $!";

sysread $bin, my $header, 4;
print "File Format: " . $header . "\n";

my $rsid = $header == 'RSID';

sysread $bin, my $version, 2;
print "Format Version: " . unpack("n", $version) . "\n";

# This is the offset from the start of the file to the C64 binary data area.
# Because of the fixed size of the header, this is either 0x0076 for version 1
# and 0x007C for version 2 and 3.
sysread $bin, my $dataOffset, 2;
printf "Data offset: \$%04x\n", unpack("n", $dataOffset);

# The C64 memory location where to put the C64 data. 0 means the data are in
# original C64 binary file format, i.e. the first two bytes of the data contain
# the little-endian load address (low byte, high byte). This must always be true
# for RSID files. Furthermore, the actual load address must NOT be less than
# $07E8 in RSID files.
sysread $bin, my $loadAddress, 2;
if (unpack("n", $loadAddress) > 0) {
	if ($rsid) {
		printf "Broken RSID tune, laod address must be 0 but was: \$%04x\n", $loadAddress
	}
	else {
		printf "Load Address: \$%04x\n", unpack("n", $loadAddress);
	}
}

sysread $bin, my $initAddress, 2;
printf "Init Address: \$%04x\n", unpack("n", $initAddress);

sysread $bin, my $playAddress, 2;
printf "Play Address: \$%04x\n", unpack("n", $playAddress);

sysread $bin, my $songs, 2;
printf "Number of tunes: %d\n", unpack("n", $songs);

sysread $bin, my $startSong, 2;
printf "Default tune: %d\n", unpack("n", $startSong);

sysread $bin, my $speed, 4;
printf "Speed: \$%08x\n", unpack("N", $speed);

sysread $bin, my $name, 32;
printf "Title: %s\n", $name;

sysread $bin, my $author, 32;
printf "Author: %s\n", $author;

sysread $bin, my $released, 32;
printf "Released: %s\n", $released;

if (unpack("n", $version) > 1) {
	sysread $bin, my $binflags, 2;
	my $flags = unpack("n", $binflags);

	printf "MUS: %s\n", ($flags & 0x01)?"true":"false";
	if ($header == "PSID") {
		printf "PlaySID Specific: %s\n", ($flags & 0x02)?"true":"false";
	} else {
		printf "BASIC: %s\n", ($flags & 0x02)?"true":"false";
	}
	
	my %clock = (
		0b00 => "Unknown",
		0b01 => "PAL",
		0b10 => "NTSC",
		0b11 => "PAL and NTSC"
	);
	printf "Clock: %s\n", $clock {($flags & 0x0C) >> 2};

	my %model = (
		0b00 => "Unknown",
		0b01 => "6581",
		0b10 => "8580",
		0b11 => "6581 and 8580"
	);
	printf "SID Model: %s\n", $model {($flags & 0x30) >> 4};

	if (unpack("n", $version) > 2) {
		printf "2nd SID Model: %s\n", $model {($flags & 0xC0) >> 6};
		if (unpack("n", $version) > 3) {
			printf "3rd SID Model: %s\n", $model {($flags & 0x300) >> 8};
		}
	}

	sysread $bin, my $startPage, 1;
	printf "Start Page: \$%02x\n", unpack("C", $startPage);

	sysread $bin, my $pageLength, 1;
	printf "Page Length: \$%02x\n", unpack("C", $pageLength);

	sysread $bin, my $secondSIDAddress, 1;
	sysread $bin, my $thirdSIDAddress, 1;
	if (unpack("n", $version) > 2) {
		printf "2nd SID Address: \$D%02x0\n", unpack("C", $secondSIDAddress);
		if (unpack("n", $version) > 3) {
			printf "3rd SID Address: \$D%02x0\n", unpack("C", $thirdSIDAddress);
		}
	}
}

if (unpack("n", $loadAddress) == 0) {
	sysread $bin, my $realLoadAddr, 2;
	if ($rsid && unpack("v", $realLoadAddr) < 0x07E8) {
		printf "Broken RSID tune, laod address must be at least \$07E8 but was: \$%04x\n", $realLoadAddr
	}
	else {
		printf "Load Address: \$%04x\n", unpack("v", $realLoadAddr);
	}
}

# Calculate md5 sum

my $ctx = Digest::MD5->new;

# Include C64 data
my $offset = unpack("n", $dataOffset);
if (unpack("n", $loadAddress) == 0) {
    $offset += 2;
}
sysseek $bin, $offset, SEEK_SET;
do {
	my $datalen = sysread $bin, my $buffer, 65536;
	$ctx->add($buffer);
} while ($datalen > 0);

# Include INIT and PLAY address
$ctx->add(pack("v", unpack("n", $initAddress)));
$ctx->add(pack("v", unpack("n", $playAddress)));

# # Include number of songs
$ctx->add(pack("v", unpack("n", $songs)));

# Include song speed for each song
my $mask = 1;
for (my $i = 1; $i <= unpack("n", $songs); $i++, $mask <<= 1) {
    if (unpack("N", $speed) & $mask) {
        $ctx->add(pack("c", 1));
    } else {
        $ctx->add(pack("c", 0));
    }
}

# Include clock speed flags
if (($flags & 0x0C) >> 2 == 0b10) {
    $ctx->add(ack("c", 2));
}

print $ctx->hexdigest;
print "\n";


close $bin;
