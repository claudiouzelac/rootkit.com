DIRS= \
     engines\~   \
     sample\~


ALL : $(DIRS)


$(DIRS):
    @if exist $(@D)\makefile <<nmaketmp.bat
    @cd $(@D)
    @echo ** Building $(@D) **
    @$(MAKE) -nologo
    @cd ..
<<

