## common/Makefile.am

## Convinience library:
noinst_LTLIBRARIES = libcommon.la
libcommon_la_SOURCES = ConfigFile.cpp ConfigFile.hpp \
		       GeneralEvents.hpp \
		       MediaCommon.cpp MediaCommon.hpp
libcommon_la_CXXFLAGS = -std=c++0x

## Parser/Generator Test
noinst_PROGRAMS = configFileTest
configFileTest_SOURCES = ConfigFile.cpp ConfigFile.hpp \
			 ConfigFileTest.cpp ConfigFileTest.hpp
configFileTest_CPPFLAGS = -DCONFIG_FILE_TEST $(AM_CFLAGS) $(BOOST_CPPFLAGS)
configFileTest_CXXFLAGS = -std=c++0x
configFileTest_LDFLAGS = $(BOOST_LDFLAGS)
configFileTest_LDADD = ../platform/libplatform.la \
		       $(BOOST_THREAD_LIB) $(BOOST_FILESYSTEM_LIB) \
		       -lboost_system \
		       -lz -lm
