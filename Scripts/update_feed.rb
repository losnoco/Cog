#!/usr/bin/env ruby

require 'tempfile'
require 'open-uri'
require 'shellwords'
require 'nokogiri'
require 'time'

feed = ARGV[0] || 'mercury'

signature_file = "#{Dir.home}/.ssh/dsa_priv.pem"

site_dir = "#{Dir.home}/Source/Repos/kode54-net/cog"

appcast = open("#{site_dir}/#{feed}_builds/#{feed}.xml")

appcastdoc = Nokogiri::XML(appcast)

appcast.close

sparkle = appcastdoc.namespaces['xmlns:sparkle']

channel = appcastdoc.xpath('//channel')

sortedchannels = channel.search('./item').sort_by{ |i| Time.parse(i.at('pubDate').text) }.reverse

#Get the latest revision from the appcast
appcast_enclosure = sortedchannels[0].search('./enclosure').first
appcast_url = appcast_enclosure.attribute('url');
appcast_revision = appcast_enclosure.attribute_with_ns('version', sparkle).to_s;
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
  plistdoc = Nokogiri::XML(plist)
  plist.close

  version_element = plistdoc.xpath('//key[.="CFBundleVersion"]/following-sibling::string[1]')

  latest_revision = version_element.inner_text

#latest_revision = %x[/usr/local/bin/hg log -r . --template '{latesttag}-{latesttagdistance}-{node|short}']
revision_split = latest_revision.split( /-/ )
revision_number = revision_split[1]
revision_code = revision_split[2]
revision_split = revision_code.split( /g/ )
revision_code = revision_split[1]

if appcast_revision < latest_revision
  #Get the changelog
  changelog = %x[/usr/bin/git log #{appcast_revision_code}..#{revision_code} --pretty=format:'<li> <a href="https://gitlab.kode54.net/kode54/Cog/commit/%H">view commit</a> &bull; %s</li> ' --reverse]

  description = changelog

  filename = "Cog-#{revision_code}.zip"
  filenamedesc = "Cog-#{revision_code}.html"
  deltamask = "Cogk54-#{revision_number}-"
  temp_path = "/tmp";
  %x[rm -rf '#{temp_path}/Cog.app' '#{temp_path}/Cog.zip']
  
  #Copy the replacement build
  %x[cp -R '#{app_path}/Cog.app' '#{temp_path}/Cog.app']

  #Sign it!
  %x[codesign -s 'Developer ID Application' -f '#{temp_path}/Cog.app']

  #Zip the app!
  %x[rm -f '#{temp_path}/#{feed}.zip']
  %x[ditto -c -k --sequesterRsrc --keepParent --zlibCompressionLevel 9 '#{temp_path}/Cog.app' '#{temp_path}/#{feed}.zip']
  
  #Send the new build to the storage path
  %x[cp '#{temp_path}/#{feed}.zip' '#{site_dir}/#{feed}_builds/#{filename}']
  %x[rm '#{temp_path}/#{feed}.zip']

  #Generate the description document
  descriptiondoc = Tempfile.new('#{feed}.html')
  descriptiondoc.write(description)
  descriptiondoc.close()

  #Send it to the storage path
  %x[cp '#{descriptiondoc.path}' '#{site_dir}/#{feed}_builds/#{filenamedesc}']
  %x[rm '#{descriptiondoc.path}']

  #Update appcast
  %x[generate_appcast '#{signature_file}' '#{site_dir}/#{feed}_builds']

  #List out the deltas
  deltas = Dir.entries("#{site_dir}/#{feed}_builds").select { |f| f =~ /\A#{Regexp.escape(deltamask)}.+\.delta\z/ }.map { |f| File.join("#{site_dir}/#{feed}_builds", f) }

  #Upload them to S3
  %x[s3cmd put -P -m application/octet-stream #{deltas.shelljoin} '#{site_dir}/#{feed}_builds/#{filename}' s3://balde.losno.co/cog/ --signature-v2]
 
  #Clean up
  %x[rm -rf '#{temp_path}/Cog.app']

  #Upload to S3
  %x[s3cmd put -P -m application/xml '#{site_dir}/#{feed}_builds/#{feed}.xml' s3://balde.losno.co/cog/ --signature-v2]

  #Invalidate CDN
  %x[aws cloudfront create-invalidation --distribution-id E1WUF11KRNP2VH --paths /cog/mercury.xml]
end
