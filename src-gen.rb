#!/usr/bin/env ruby

ARGV.each { |mod_name|
  base = File.basename mod_name
  c_filename = "#{mod_name}.c"
  h_filename = "#{mod_name}.h"
  guard_name = base.upcase.gsub(/[^a-zA-Z0-9]+/, '_')
  h_guard = "__SNOW__#{guard_name}_H__"
  c_guard = "__SNOW__#{guard_name}_C__"

C_TEMPLATE = <<-EOS
#define #{c_guard}

#include "#{h_filename}"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



#ifdef __cplusplus
}
#endif /* __cplusplus */
EOS

H_TEMPLATE = <<-EOS
#ifndef #{h_guard}
#define #{h_guard} 1

/* Includes */
#include <snow-config.h>

/* Inline boilerplate */
#ifdef #{c_guard}
#define S_INLINE
#else
#define S_INLINE inline
#endif /* #{c_guard} */
/* End inline boilerplate */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



#ifdef __cplusplus
}
#endif /* __cplusplus */

/* Undefine S_INLINE */
#include <inline.end>

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
