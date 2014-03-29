#!/usr/bin/env ruby

require 'tempfile'
require 'open-uri'
require 'rexml/document'
include REXML

feed = ARGV[0] || 'mercury'

upload_prefix = "Chris@kode54.net:/usr/share/nginx/html/cog/"

signature_file = "#{Dir.home}/.ssh/dsa_priv.pem"

appcast = open("https://kode54.net/cog/#{feed}.xml")

appcastdoc = Document.new(appcast)

#Get the latest revision from the appcast
appcast_enclosure = REXML::XPath.match(appcastdoc, "//channel/item/enclosure")[0]
appcast_url = appcast_enclosure.attributes['url'];
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

  filename = "Cog-#{revision_code}.zip"
  filename_delta = "Cog-#{revision_code}.delta"
  temp_path = "/tmp";
  %x[rm -rf '#{temp_path}/Cog.app' '#{temp_path}/Cog.old' '#{temp_path}/Cog.zip']
  
  #Retrieve the current full package
  %x[curl -o '#{temp_path}/Cog.zip' #{appcast_url}]
  
  #Unpack and rename
  %x[ditto -kx '#{temp_path}/Cog.zip' '#{temp_path}/']
  %x[mv '#{temp_path}/Cog.app' '#{temp_path}/Cog.old']
  
  #Copy the replacement build
  %x[cp -R '#{app_path}/Cog.app' '#{temp_path}/Cog.app']

  #Sign it!
  %x[#{File.expand_path("../fucking_sign_it.sh", __FILE__)} '#{temp_path}/Cog.app']

  #Zip the app!
  %x[rm -f '#{temp_path}/#{feed}.zip']
  %x[ditto -c -k --sequesterRsrc --keepParent --zlibCompressionLevel 9 '#{temp_path}/Cog.app' '#{temp_path}/#{feed}.zip']
  
  #Generate delta patch
  %x[BinaryDelta create '#{temp_path}/Cog.old' '#{temp_path}/Cog.app' '#{temp_path}/#{feed}.delta']

  filesize = File.size("#{temp_path}/#{feed}.zip")
  filesize_delta = File.size("#{temp_path}/#{feed}.delta")

  openssl = "/usr/bin/openssl"
  signature_delta = `#{openssl} dgst -sha1 -binary < "#{temp_path}/#{feed}.delta" | #{openssl} dgst -dss1 -sign "#{signature_file}" | #{openssl} enc -base64`

  #Send the new build to the server
  %x[scp '#{temp_path}/#{feed}.zip' #{upload_prefix}#{feed}_builds/#{filename}]
  %x[rm '#{temp_path}/#{feed}.zip']
  
  #Send the delta
  %x[scp '#{temp_path}/#{feed}.delta' #{upload_prefix}#{feed}_builds/#{filename_delta}]
  %x[rm '#{temp_path}/#{feed}.delta']
  
  #Clean up
  %x[rm -rf '#{temp_path}/Cog.old' '#{temp_path}/Cog.app']

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
  
  new_item.add_element('sparkle:deltas')
  new_item.elements['sparkle:deltas'].add_element('enclosure')
  new_item.elements['sparkle:deltas'].elements['enclosure'].add_attribute('url', "https://kode54.net/cog/#{feed}_builds/#{filename_delta}")
  new_item.elements['sparkle:deltas'].elements['enclosure'].add_attribute('length', filesize_delta)
  new_item.elements['sparkle:deltas'].elements['enclosure'].add_attribute('type', 'application/octet-stream')
  new_item.elements['sparkle:deltas'].elements['enclosure'].add_attribute('sparkle:version', "#{latest_revision}")
  new_item.elements['sparkle:deltas'].elements['enclosure'].add_attribute('sparkle:deltaFrom', "#{appcast_revision}")
  new_item.elements['sparkle:deltas'].elements['enclosure'].add_attribute('sparkle:dsaSignature', "#{signature_delta}")
  
  appcastdoc.insert_before('//channel/item', new_item)
  
  #Limit number of entries to 5
  appcastdoc.delete_element('//channel/item[position()>5]')
  
  new_xml = Tempfile.new('appcast.xml')
  appcastdoc.write(new_xml)
  new_xml.close()
  appcast.close()

  #Send the updated appcast to the server
  %x[scp #{new_xml.path} #{upload_prefix}#{feed}.xml]
end
