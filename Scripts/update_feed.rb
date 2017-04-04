#!/usr/bin/env ruby

require 'tempfile'
require 'open-uri'
require 'rexml/document'
include REXML

feed = ARGV[0] || 'mercury'

signature_file = "#{Dir.home}/.ssh/dsa_priv.pem"

site_dir = "#{Dir.home}/Source/Repos/kode54-net/cog"

appcast = open("#{site_dir}/#{feed}.xml")

appcastdoc = Document.new(appcast)

#Get the latest revision from the appcast
appcast_enclosure = REXML::XPath.match(appcastdoc, "//channel/item/enclosure")[0]
appcast_url = appcast_enclosure.attributes['url'];
appcast_revision = appcast_enclosure.attributes['sparkle:version'];
appcast_revision_split = appcast_revision.split( /-/ )
appcast_revision_code = appcast_revision_split[2]
appcast_revision_split = appcast_revision_code.split( /g/ )
appcast_revision_code = appcast_revision_split[1]

#Remove modified files that may cause conflicts.
#%x[hg revert --all]

#Update to the latest revision
#%x[hg pull -u]

#  %[xcodebuild -workspace Cog.xcodeproj/project.xcworkspace -scheme Cog archive 2>&1].each_line do |line|
#    if line.match(/\*\* BUILD FAILD \*\*/)
#      exit
#    end
#  end

#  archivedir = "~/Library/Developer/Xcode/Archives"
#  latest_archive = %x[find #{archivedir} -type d -name 'Cog *.xcarchive' -print0 | xargs -0 stat -f "%m %N" -t "%Y" | sort -r | head -n1 | sed -E 's/^[0-9]+ //'].rstrip
#  app_path = "#{latest_archive}/Products#{ENV['HOME']}/Applications"
  script_path = File.expand_path(File.dirname(__FILE__))
  app_path = "#{script_path}/build/Build/Products/Release"

  plist = open("#{app_path}/Cog.app/Contents/Info.plist")
  plistdoc = Document.new(plist)

  version_element = plistdoc.elements["//[. = 'CFBundleVersion']/following-sibling::string"];
  latest_revision = version_element.text

#latest_revision = %x[/usr/local/bin/hg log -r . --template '{latesttag}-{latesttagdistance}-{node|short}']
revision_split = latest_revision.split( /-/ )
revision_code = revision_split[2]
revision_split = revision_code.split( /g/ )
revision_code = revision_split[1]

if appcast_revision < latest_revision
  #Get the changelog
  changelog = %x[/usr/bin/git shortlog #{appcast_revision_code}..#{revision_code}]

  description = changelog

  filename = "Cog-#{revision_code}.zip"
  filename_delta = "Cog-#{revision_code}.delta"
  temp_path = "/tmp";
  %x[rm -rf '#{temp_path}/Cog.app' '#{temp_path}/Cog.old' '#{temp_path}/Cog.zip']
  
  #Retrieve the current full package
  local_file = appcast_url.gsub(/https:\/\/f\.losno\.co\/cog/, "#{site_dir}/#{feed}_builds")
  %x[cp '#{local_file}' '#{temp_path}/Cog.zip']
  
  #Unpack and rename
  %x[ditto -kx '#{temp_path}/Cog.zip' '#{temp_path}/']
  %x[mv '#{temp_path}/Cog.app' '#{temp_path}/Cog.old']
  
  #Copy the replacement build
  %x[cp -R '#{app_path}/Cog.app' '#{temp_path}/Cog.app']

  #Sign it!
  %x[codesign -s 'Developer ID Application' -f '#{temp_path}/Cog.app']

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
  %x[cp '#{temp_path}/#{feed}.zip' '#{site_dir}/#{feed}_builds/#{filename}']
  %x[rm '#{temp_path}/#{feed}.zip']
  
  #Send the delta
  %x[cp '#{temp_path}/#{feed}.delta' '#{site_dir}/#{feed}_builds/#{filename_delta}']
  %x[rm '#{temp_path}/#{feed}.delta']
 
  #Upload them to S3
  %x[s3cmd put -P -m application/octet-stream '#{site_dir}/#{feed}_builds/#{filename_delta}' '#{site_dir}/#{feed}_builds/#{filename}' s3://balde.losno.co/cog/ --signature-v2]
 
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
  new_item.elements['enclosure'].add_attribute('url', "https://f.losno.co/cog/#{filename}")
  new_item.elements['enclosure'].add_attribute('length', filesize)
  new_item.elements['enclosure'].add_attribute('type', 'application/octet-stream')
  new_item.elements['enclosure'].add_attribute('sparkle:version', "#{latest_revision}")
  
  new_item.add_element('sparkle:deltas')
  new_item.elements['sparkle:deltas'].add_element('enclosure')
  new_item.elements['sparkle:deltas'].elements['enclosure'].add_attribute('url', "https://f.losno.co/cog/#{filename_delta}")
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
  %x[cp '#{new_xml.path}' '#{site_dir}/#{feed}.xml']

  #Upload to S3
  %x[s3cmd put -P -m application/xml '#{site_dir}/#{feed}.xml' s3://balde.losno.co/cog/ --signature-v2]

  #Invalidate CDN
  %x[aws cloudfront create-invalidation --distribution-id E1WUF11KRNP2VH --paths /cog/mercury.xml]
end
