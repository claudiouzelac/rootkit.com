DIRS= \
     catchy\~   \
     pullout\~   \
     phide2\~


ALL : $(DIRS)


$(DIRS):
    @if exist $(@D)\makefile <<nmaketmp.bat
    @cd $(@D)
    @echo *** Building engine: $(@D) ***
    @$(MAKE) -nologo
    @copy /b Release\*.lib ..\..\sample\ >nul
    @cd ..
<<

