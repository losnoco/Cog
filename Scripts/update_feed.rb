#!/usr/bin/env ruby

require 'tempfile'
require 'open-uri'
require 'rexml/document'
include REXML

feed = ARGV[0] || 'mercury'

appcast = open("https://kode54.net/cog/#{feed}.xml")

appcastdoc = Document.new(appcast)

#Get the latest revision from the appcast
appcast_enclosure = REXML::XPath.match(appcastdoc, "//channel/item/enclosure")[0]
appcast_revision = appcast_enclosure.attributes['sparkle:version'];
appcast_revision_split = appcast_revision.split( /-/ )
appcast_revision_code = appcast_revision_split[2]

#Remove modified files that may cause conflicts.
#%x[hg revert --all]

#Update to the latest revision
#%x[hg pull -u]

#  %[xcodebuild -workspace Cog.xcodeproj/project.xcworkspace -scheme Cog archive 2>&1].each_line do |line|
#    if line.match(/\*\* BUILD FAILD \*\*/)
#      exit
#    end
#  end

  archivedir = "~/Library/Developer/Xcode/Archives"
  latest_archive = %x[find #{archivedir} -type d -name 'Cog *.xcarchive' -print0 | xargs -0 stat -f "%m %N" -t "%Y" | sort -r | head -n1 | sed -E 's/^[0-9]+ //'].rstrip
  app_path = "#{latest_archive}/Products#{ENV['HOME']}/Applications"

  plist = open("#{app_path}/Cog.app/Contents/Info.plist")
  plistdoc = Document.new(plist)

  version_element = plistdoc.elements["//[. = 'CFBundleVersion']/following-sibling::string"];
  latest_revision = version_element.text

#latest_revision = %x[/usr/local/bin/hg log -r . --template '{latesttag}-{latesttagdistance}-{node|short}']
revision_split = latest_revision.split( /-/ )
revision_code = revision_split[2]

if appcast_revision < latest_revision
  #Get the changelog
  changelog = %x[/usr/local/bin/hg log --template '{desc}\\n' -r #{revision_code}:children\\(#{appcast_revision_code}\\)]

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

  filename = "Cog-#{revision_code}.tbz"

  #Sign it!
  %x[codesign -s 'Developer ID Application' --deep --force '#{app_path}/Cog.app']

  #Zip the app!
  %x[rm -f /tmp/#{feed}.tar.bz2]
  %x[tar -C '#{app_path}' -cjf /tmp/#{feed}.tar.bz2 Cog.app]

  filesize = File.size("/tmp/#{feed}.tar.bz2")

  #Send the new build to the server
  %x[scp /tmp/#{feed}.tar.bz2 ec2-user@kode54.net:/usr/share/nginx/html/cog/#{feed}_builds/#{filename}]
  %x[rm /tmp/#{feed}.tar.bz2]

  #Add new entry to appcast
  new_item = Element.new('item')
  
  new_item.add_element('title')
  new_item.elements['title'].text = "Version 0.08 (#{latest_revision})"
  
  new_item.add_element('description')
  new_item.elements['description'].text = description

  new_item.add_element('pubDate')
  new_item.elements['pubDate'].text = %x[date -r `stat -f "%m" '#{app_path}'` +'%a, %d %b %Y %T %Z'].rstrip #RFC 822
  
  new_item.add_element('sparkle:minimumSystemVersion')
  new_item.elements['sparkle:minimumSystemVersion'].text =  '10.7.0'

  new_item.add_element('enclosure')
  new_item.elements['enclosure'].add_attribute('url', "https://kode54.net/cog/#{feed}_builds/#{filename}")
  new_item.elements['enclosure'].add_attribute('length', filesize)
  new_item.elements['enclosure'].add_attribute('type', 'application/octet-stream')
  new_item.elements['enclosure'].add_attribute('sparkle:version', "#{latest_revision}")
  
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
