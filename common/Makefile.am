## common/Makefile.am


## Convinience library:
noinst_LTLIBRARIES    = libcommon.la
libcommon_la_SOURCES = ConfigFile.cpp MediaCommon.cpp

## Parser/Generator Test
bin_PROGRAMS = configFileTest
configFileTest_SOURCES = ConfigFile.cpp ConfigFileTest.cpp
configFileTest_CPPFLAGS = -DCONFIG_FILE_TEST $(AM_CFLAGS) $(BOOST_CPPFLAGS)
configFileTest_LDADD = ../platform/libplatform.la
configFileTest_LDFLAGS = -lz -lm $(BOOST_LDFLAGS) $(BOOST_THREAD_LIB) $(BOOST_FILESYSTEM_LIB)
