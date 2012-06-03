#!/usr/local/bin/ruby -w

COMPILE_STMT = "\t$(TARGET_CC) $(TARGET_CFLAGS) $(CFLAGS) -c -o $@ $<"

INCLUDE_FILE = /^\s*#\s*include\s+["<]([^">]+)[">"]/
SOURCE_FILE = /\.c(c|pp|xx)?$/

def pullPrerequisites(file, checks, include_paths=[])
	if (checks.nil?)
		checks = [file]
	else
		checks << file
	end
	
	prereqs = []
	if (!File.exists?(file)) 
		Process.exit(false)
	end
	
	fileIn = File.new(file)
	
	if (not fileIn.nil?)
		fileIn.each_line {
			|line|
		
			matches = line.match(INCLUDE_FILE)
			next if (matches.nil?)
		
			found = matches[1]
			prereqs << found
		}
	else
		puts "Error loading file #{file}"
	end
	
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

def scan_sources(path, sources, include_paths=[])
	Dir.entries(path).each(path) {
		|entry|

		next if (entry =~ SOURCE_FILE).nil?
		fpath = "#{path}/#{entry}"

		sources[fpath] = pull_prereqs(entry, nil, include_paths)
	}

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
	outString = "$(addprefix $(OBJECT_PREFIX),#{basename}.o):"
	
	if (!prereqs.empty?) then
		outString += " #{prereqs.join(" ")}"
	end
	
	print "\n#{outString}\n#{COMPILE_STMT}\n"
}
