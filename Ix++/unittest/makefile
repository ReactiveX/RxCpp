

!ifndef Config
Config=Debug
!endif

O=..\..\bin\$(Config)

!message Building ===== $(Config) =====

program=testbench.exe
INCLUDE=$(INCLUDE);../src

!if "$(Config)"=="Debug"
OPTIONS=/Od 
!else
OPTIONS=/Ox
!endif
OPTIONS=$(OPTIONS) /Zi /I$(BOOST) /DBOOST_RESULT_OF_USE_DECLTYPE

runtests : "$O/$(program)"
  "$O/$(program)"


all : "$O/$(program)"

$O : 
  mkdir $O

"$O/$(program)" : testbench.cpp testbench.hpp ../src/cpplinq/*.hpp $O
  $(CPP) $(OPTIONS) /EHsc /Zi /Fe"$@" /Fo$O/ testbench.cpp 

clean :
  del /Q $O\*.exe
  del /Q $O\*.pdb
  del /Q $O\*.obj
  del /Q $O\*.ilk
