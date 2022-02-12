add_library(kaki-common INTERFACE include/overloaded.h include/cereal_ext.h include/membuf.h)
target_include_directories(kaki-common INTERFACE include)
target_link_libraries(kaki-common INTERFACE cereal)