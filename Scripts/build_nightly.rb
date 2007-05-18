#!/sw/bin/ruby

require 'tempfile'
require 'open-uri'
require 'rexml/document'
include REXML

appcast = open('http://cogx.org/appcast/nightly.xml')

doc = Document.new(appcast)

#Get the latest revision from the appcast
appcast_revision = Regexp.new('\d+$').match(doc.elements['//channel/item/title'].text.to_s()).to_s().to_i() || 0

#Update to the latest revision
latest_revision = %x[svn update | tail -n 1].gsub(/[^\d]+/, '').to_i()

if appcast_revision < latest_revision
	#Get the changelog
	changelog = %x[svn log -r #{latest_revision}:#{appcast_revision+1}]

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

	#Send the new build to the server
	%x[scp build/Release/#{filename} cogx@cogx.org:~/cogx.org/nightly_builds/]

  #Add new entry to appcast
  new_item = Element.new('item')
  
  new_item.add_element('title')
  new_item.elements['title'].text = "Cog r#{latest_revision}"
  
  new_item.add_element('description')
  new_item.elements['description'].text = changelog

  new_item.add_element('pubDate')
  new_item.elements['pubDate'].text = Time.now().strftime("%a, %d %b %Y %H:%M:%S %Z") #RFC 822
  
  new_item.add_element('enclosure')
  new_item.elements['enclosure'].add_attribute('url', "http://cogx.org/nightly_builds/#{filename}")
  new_item.elements['enclosure'].add_attribute('length', filesize)
  new_item.elements['enclosure'].add_attribute('type', 'application/octet-stream')
  
  doc.insert_before('//channel/item', new_item)
  
  #Limit number of entries to 5
  doc.delete_element('//channel/item[position()>5]')
  
  new_xml = Tempfile.new('appcast.xml')
	new_xml << doc.to_s()
	new_xml.close()
	appcast.close()

	#Send the updated appcast to the server
	%x[scp #{new_xml.path} cogx@cogx.org:~/cogx.org/appcast/nightly.xml]
	
end




