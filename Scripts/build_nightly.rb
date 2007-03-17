#!/sw/bin/ruby

require 'yaml'
require 'erb'
include ERB::Util

yaml_file = 'Scripts/nightlies.yaml'

#Load yaml and get the current revision
if File.exists?(yaml_file)
	entries = YAML::load(File.open(yaml_file))
	local_revision = entries[0]['revision'].to_i()
else
	entries = []
	local_revision = 0
end

#Update to the latest revision
latest_revision = %x[svn update | tail -n 1].gsub(/[^\d]+/, '').to_i()

if local_revision < latest_revision
	#Get the changelog
	changelog = %x[svn log -r #{latest_revision}:#{local_revision+1}]

	#Remove the previous build directories
	%x[find . -type d -name build -print0 | xargs -0 rm -r ]

	#Build Cog!
	%x[./Scripts/build_cog.sh].each_line do |line|
		if line.match(/\*\* BUILD FAILED \*\*/)
			exit
		end
	end

	filename = "Cog-r#{latest_revision}.tbz2"

	#Zip the app!
	%x[tar cjf build/Release/#{filename} build/Release/Cog.app]

	filesize = File.size("build/Release/#{filename}")

	#Update yaml
	entry = {
		'revision' => latest_revision,
		'changelog' => changelog,
		'filename' => filename,
		'filesize' => filesize,
		'date' => Time.now().strftime("%a, %d %b %Y %H:%M:%S %Z") #RFC 822
	}

	#Send the new build to the server
	%x[scp build/Release/#{filename} vince@vspader.com:~/public_html/]

	entries.insert(0, entry)
	File.open(yaml_file, 'w') do |output|
		output << entries.to_yaml()
	end

	#Build the appcast from the template!
	appcast = ERB.new(File.open('Scripts/appcast.erb'), 0, '<>')

	File.open('Scripts/appcast.xml', 'w') do |output|
		output <<  appcast.result()
	end

	#Send the updated appcast to the server
	%x[scp Scripts/appcast.xml vince@vspader.com:~/public_html/]
end




