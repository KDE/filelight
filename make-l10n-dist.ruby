#!/usr/bin/env ruby
# adopted from release_amarok.rb

#===============================================================================
# edit these to customise behaviour
#===============================================================================
branch = 'trunk'
user = 'mhowell'
protocol = 'https'
category = 'utils'
product = 'filelight'
#===============================================================================




#===============================================================================
# header
#===============================================================================

def mkdir( dirname )
   begin
      Dir.mkdir dirname
   rescue Errno::EEXIST
      # ignore error if directory exists
   end

   return Dir.new( dirname )
end




#===============================================================================
# checkout and generate build system
#===============================================================================

# create our root directory
mkdir( "#{product}-#{version}-i18n-#{Date.today.chomp '-'}" ).chdir()

# get the vital buildsystem bits
`svn co #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/extragear/#{category}/Makefile.cvs`
`svn co #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/extragear/#{category}/configure.in`
`svn co #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/l10n/COPYING`
`svn co #{protocol}://#{user}@svn.kde.org/home/kde/branches/KDE/3.5/kde-common/admin`

# make some more subdirectorries
mkdir( "doc" )
mkdir( "po" )
mkdir( "l10n" ).chdir();


# list of available languages
languages = `svn cat #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/l10n/subdirs`


# docs
for lang in languages
   lang.chomp!()
   `rm -rf #{product}`
   dirname = "l10n/#{lang}/docs/extragear-#{category}/#{product}"
   `svn co -q #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/#{dirname} > /dev/null 2>&1`
   next unless FileTest.exists?( product )

   print "Copying #{product} documentation for #{lang}...  "
   `cp -R #{product}/ ../doc/#{lang}`

   # we don't want KDE_DOCS = AUTO, because that makes the
   # build system assume that the product of the app is the
   # same as the product of the dir the Makefile.am is in.
   # Instead, we explicitly pass the product..
   makefile = File.new( "../doc/#{lang}/Makefile.am", File::CREAT | File::RDWR | File::TRUNC )
   makefile << "KDE_LANG = #{lang}\n"
   makefile << "KDE_DOCS = #{product}\n"
   makefile.close()

   puts "done"
end


Dir.chdir( ".." )
puts "\n"


# po
for lang in languages
   lang.chomp!()
   filename = "l10n/#{lang}/messages/extragear-#{category}/#{product}.po"
   `svn cat #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/#{filename} 2> /dev/null | tee l10n/#{product}.po`
   next if FileTest.size( "l10n/#{product}.po" ) == 0

   dest = "po/#{lang}"
   Dir.mkdir( dest )
   print "Copying #{product}.po for #{lang}...  "
   `mv l10n/#{product}.po #{dest}`

   makefile = File.new( "#{dest}/Makefile.am", File::CREAT | File::RDWR | File::TRUNC )
   makefile << "KDE_LANG = #{lang}\n"
   makefile << "SUBDIRS  = $(AUTODIRS)\n"
   makefile << "POFILES  = AUTO\n"
   makefile.close()

   puts "done"
end


# create crucial Makefile.am files
for dir in [po doc]
   makefile = File.new( "#{dir}/Makefile.am", File::CREAT | File::RDWR | File::TRUNC )
   makefile << "SUBDIRS = $(AUTODIRS)\n"
   makefile.close()make
end

makefile = File.new( "Makefile.am", File::CREAT | File::RDWR | File::TRUNC )
makefile << "AUTOMAKE_OPTIONS = foreign"
makefile << "SUBDIRS  = doc po\n"
makefile.close()




#===============================================================================
# cleanup
#===============================================================================

`mkdir delete_me`
`mv po/xx delete_me`
`mv l10n delete_me/#{product}.l10n.backup.#{Date.today.to_s}`




#===============================================================================
# autoconf/automake and finalise
#===============================================================================

dot_svn = `find -name .svn -exec`
`rm -rf #{dot_svn}`
ENV["UNSERMAKE"] = 'no'
ENV["AUTOMAKE"] = '/usr/bin/automake-1.6'
# we could avoid these if we could use automake --foreign
#`touch COPYING README NEWS AUTHORS INSTALL ChangeLog`
`make -f Makefile.cvs`
`rm -rf autom4te.cache stamp.h.in Makefile.cvs`
yesterday = Date.today - 1
`find -exec touch --date=#{yesterday.to_s} {} \;`
