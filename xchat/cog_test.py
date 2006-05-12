import commands
import string

def cog_test():
	command = "osascript -e \\\n\
			\"tell application \\\"Cog\\\"\n\
 				set this_title to the title of the currententry\n\
  				set this_artist to the artist of the currententry\n\
				set this_album to the album of the currententry\n\
				set this_bitrate to the bitrate of the currententry\n\
				set this_length to the length of the currententry\n\
			end tell\n\
			return this_title & tab & this_artist & tab & this_album & tab & this_bitrate & tab & this_length\""
	
	output = commands.getoutput(command);

	info = string.split(output,"\t")
	length = float(info[4])
	length = int(length/1000)
	min = length / 60
	sec = length % 60
	line = "[ Artist: %s ][ Album: %s ][ Title: %s ][ %skbps ][ %i:%02i ]" % (info[1], info[2], info[0], info[3], min, sec)

	print line

cog_test();
