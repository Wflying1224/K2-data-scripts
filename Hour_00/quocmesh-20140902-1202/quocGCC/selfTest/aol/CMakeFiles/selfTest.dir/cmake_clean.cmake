FILE(REMOVE_RECURSE
  "CMakeFiles/selfTest"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/selfTest.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
