#!/usr/bin/env ruby

require 'tempfile'
require 'open-uri'
require 'rexml/document'
include REXML

feed = ARGV[0] || 'mercury'

appcast = open("https://kode54.net/cog/#{feed}.xml")

appcastdoc = Document.new(appcast)

#Get the latest revision from the appcast
appcast_revision = appcastdoc.elements['//channel/item/enclosure/sparkle:version'].text.to_s() || 0
appcast_revision_split = appcast_revision.split( /-/ )
appcast_revision_code = appcast_revision_split[2]

#Remove modified files that may cause conflicts.
#%x[hg revert --all]

#Update to the latest revision
#%x[hg pull -u]
latest_revision = %x[/usr/local/bin/hg log -r . --template '{latesttag}-{latesttagdistance}-{node|short}']
revision_split = latest_revision.split( /-/ )
revision_code = revision_split[2]

if appcast_revision < latest_revision
  #Get the changelog
  changelog = %x[hg log --template '{desc}\n' -r #{revision_code}:children(#{appcast_revision_code})]

  description = ''
  ignore_next = false
  changelog.each_line do |line|
    if (ignore_next)
      ignore_next = false
      next
    end
    if Regexp.new('^-+$').match(line)
      ignore_next = true
      next
    elsif Regexp.new('^\s*$').match(line)
      next
    end
    description += line
  end
  
  #Remove the previous build directories
  %x[find . -type d -name build -print0 | xargs -0 rm -r ]

  #Build Cog!
  %x[xcodebuild -alltargets -configuration Release 2>&1].each_line do |line|
    if line.match(/\*\* BUILD FAILED \*\*/)
      exit
    end
  end

  filename = "Cog-#{revision_code}.tbz"

  #Zip the app!
  %x[rm -f build/Release/#{feed}.tar.bz2]
  %x[tar -C build/Release -cjf build/Release/#{feed}.tar.bz2 Cog.app]

  filesize = File.size("build/Release/#{feed}.tar.bz2")

  #Send the new build to the server
  %x[scp build/Release/#{feed}.tar.bz2 ec2-user@kode54.net:/usr/share/nginx/html/cog/#{feed}_builds/#{filename}]

  #Add new entry to appcast
  new_item = Element.new('item')
  
  new_item.add_element('title')
  new_item.elements['title'].text = "Cog {latest_revision}"
  
  new_item.add_element('description')
  new_item.elements['description'].text = description

  new_item.add_element('pubDate')
  new_item.elements['pubDate'].text = Time.now().strftime("%a, %d %b %Y %H:%M:%S %Z") #RFC 822
  
  new_item.add_element('sparkle:minimumSystemVersion')
  new_item.elements['sparkle:minimumSystemVersion'].text =  '10.7.0'

  new_item.add_element('enclosure')
  new_item.elements['enclosure'].add_attribute('url', "http://kode54.net/cog/#{feed}_builds/#{filename}")
  new_item.elements['enclosure'].add_attribute('length', filesize)
  new_item.elements['enclosure'].add_attribute('type', 'application/octet-stream')
  new_item.elements['enclosure'].add_attribute('sparkle:version', "{latest_revision}")
  
  appcastdoc.insert_before('//channel/item', new_item)
  
  #Limit number of entries to 5
  appcastdoc.delete_element('//channel/item[position()>5]')
  
  new_xml = Tempfile.new('appcast.xml')
  appcastdoc.write(new_xml)
  new_xml.close()
  appcast.close()
  
  #Send the updated appcast to the server
  %x[scp #{new_xml.path} ec2-user@kode54.net:/usr/share/nginx/html/cog/#{feed}.xml]
  
end
