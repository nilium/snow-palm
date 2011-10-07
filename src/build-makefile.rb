#!/usr/bin/env ruby -w

MAKEFILE_TEMPLATE = <<-EOS
OBJECT_PREFIX=../obj/$(TARGET)/
OBJECTS=$(addsuffix .o,$(basename $(SOURCES)))
OUTPUT_PREFIX=../bin/

all: prepare_build $(OUTPUT)

prepare_build:
	mkdir -p $(OBJECT_PREFIX)
	mkdir -p $(OUTPUT_PREFIX)
.PHONY: all prepare_build

$(OUTPUT): $(OBJECTS)
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) $(LDFLAGS) $(CFLAGS) $(addprefix $(OBJECT_PREFIX),$^) -o $(addprefix $(OUTPUT_PREFIX),$@)
EOS

COMPILE_STMT = "\t$(TARGET_CC) $(TARGET_CFLAGS) $(CFLAGS) -c -o $(OBJECT_PREFIX)$@ $<"

INCLUDE_FILE = /^\s*#\s*include\s+"([^"]+)"/
SOURCE_FILE = /\.c$/

def pullPrerequisites(file, checks)
	if (checks.nil?)
		checks = [file]
	else
		checks << file
	end
	
	prereqs = []
	fileIn = File.new(file)
	
	fileIn.each_line {
		|line|
		
		matches = line.match(INCLUDE_FILE)
		next if (matches.nil?)
		
		found = matches[1]
		prereqs << found
	}
	
	fileIn.close()
	
	prereqs.each {
		|req|
		
		if (File.basename(req) == File.basename(file) and req != file)
			prereqs.delete(req)
			prereqs.insert(0, req)
		end
		
		if (!checks.include?(req)) then
			checks << req
			prereqs += pullPrerequisites(req, checks)
		end
	}
	
	return prereqs.uniq()
end

sources = {}

Dir.entries(".").each {
	|entry|
	
	next if ((entry =~ SOURCE_FILE).nil?)
	
	sources[entry] = pullPrerequisites(entry, nil)
	sources[entry].insert(0, entry)
}

print "SOURCES=#{sources.keys.join(" \\\n        ")}\n\n"

puts MAKEFILE_TEMPLATE

sources.each {
	|file, prereqs|
	
}

sources.each {
	|file, prereqs|
	
	basename = File.basename(file, File.extname(file))
	outString = "#{basename}.o:"
	
	if (!prereqs.empty?) then
		outString += " #{prereqs.join(" ")}"
	end
	
	print "\n#{outString}\n#{COMPILE_STMT}\n"
}
