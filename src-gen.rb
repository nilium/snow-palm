#!/usr/bin/env ruby

ARGV.each { |mod_name|
  base = File.basename mod_name
  c_filename = "#{mod_name}.c"
  h_filename = "#{mod_name}.h"
  guard_name = base.upcase.gsub(/[_ \.!@#\$\%\^&*()\-=+]+/, '_')
  h_guard = "__#{guard_name}_H__"

C_TEMPLATE = <<-EOS
#define SNOW_SOURCE

#include "#{base}.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus



#ifdef __cplusplus
}
#endif // __cplusplus
EOS

H_TEMPLATE = <<-EOS
#ifndef #{h_guard}
#define #{h_guard} 1

#include <snow-config.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus



#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* end #{h_guard} include guard */
EOS

  unless File.exists? c_filename
    puts "Writing '#{c_filename}'"
    File.open(c_filename, "w") { |io|
      io.write C_TEMPLATE
    }
  else
    puts "'#{c_filename}' already exists- skipping"
  end

  unless File.exists? h_filename
    puts "Writing '#{h_filename}'"
    File.open(h_filename, "w") { |io|
      io.write H_TEMPLATE
    }
  else
    puts "'#{c_filename}' already exists- skipping"
  end
}
