# Convert the source listing to object (.obj) listing in
# another NMake Makefile module, include it, and clean it up.
# This is a "fact-of-life" regarding NMake Makefiles...
# This file does not need to be changed unless one is maintaining the NMake Makefiles

# For those wanting to add things here:
# To add a list, do the following:
# # $(description_of_list)
# if [call create-lists.bat header $(makefile_snippet_file) $(variable_name)]
# endif
#
# if [call create-lists.bat file $(makefile_snippet_file) $(file_name)]
# endif
#
# if [call create-lists.bat footer $(makefile_snippet_file)]
# endif
# ... (repeat the if [call ...] lines in the above order if needed)
# !include $(makefile_snippet_file)
#
# (add the following after checking the entries in $(makefile_snippet_file) is correct)
# (the batch script appends to $(makefile_snippet_file), you will need to clear the file unless the following line is added)
#!if [del /f /q $(makefile_snippet_file)]
#!endif

# In order to obtain the .obj filename that is needed for NMake Makefiles to build DLLs/static LIBs or EXEs, do the following
# instead when doing 'if [call create-lists.bat file $(makefile_snippet_file) $(file_name)]'
# (repeat if there are multiple $(srcext)'s in $(source_list), ignore any headers):
# !if [for %c in ($(source_list)) do @if "%~xc" == ".$(srcext)" @call create-lists.bat file $(makefile_snippet_file) $(intdir)\%~nc.obj]
#
# $(intdir)\%~nc.obj needs to correspond to the rules added in build-rules-msvc.mak
# %~xc gives the file extension of a given file, %c in this case, so if %c is a.cc, %~xc means .cc
# %~nc gives the file name of a given file without extension, %c in this case, so if %c is a.cc, %~nc means a

NULL=

# For CDK resources

!if [call create-lists.bat header resources_sources.mak CDK_RESOURCES]
!endif

!if [for %f in (..\cdk\resources\glsl\*.glsl) do @call create-lists.bat file resources_sources.mak %f]
!endif

!if [call create-lists.bat footer resources_sources.mak]
!endif

!if [call create-lists.bat header resources_sources.mak CTK_RESOURCES]
!endif

!if [for %f in (..\ctk\theme\Advaita\ctk.css ..\ctk\theme\Advaita\ctk-dark.css ..\ctk\theme\Advaita\ctk-contained.css ..\ctk\theme\Advaita\ctk-contained-dark.css) do @call create-lists.bat file resources_sources.mak %f]
!endif

!if [for %x in (png svg) do @(for %f in (..\ctk\theme\Advaita\assets\*.%x) do @call create-lists.bat file resources_sources.mak %f)]
!endif

!if [for %f in (..\ctk\theme\HighContrast\ctk.css ..\ctk\theme\HighContrast\ctk-inverse.css ..\ctk\theme\HighContrast\ctk-contained.css ..\ctk\theme\HighContrast\ctk-contained-inverse.css) do @call create-lists.bat file resources_sources.mak %f]
!endif

!if [for %x in (png svg) do @(for %f in (..\ctk\theme\HighContrast\assets\*.%x) do @call create-lists.bat file resources_sources.mak %f)]
!endif

!if [for %f in (..\ctk\theme\win32\ctk-win32-base.css ..\ctk\theme\win32\ctk.css) do @call create-lists.bat file resources_sources.mak %f]
!endif

!if [for %f in (..\ctk\cursor\*.png ..\ctk\gesture\*.symbolic.png ..\ctk\ui\*.ui) do @call create-lists.bat file resources_sources.mak %f]
!endif

!if [for %s in (16 22 24 32 48) do @(for %c in (actions status categories) do @(for %f in (..\ctk\icons\%sx%s\%c\*.png) do @call create-lists.bat file resources_sources.mak %f))]
!endif

!if [for %s in (scalable) do @(for %c in (status) do @(for %f in (..\ctk\icons\%s\%c\*.svg) do @call create-lists.bat file resources_sources.mak %f))]
!endif

!if [for %f in (..\ctk\inspector\*.ui ..\ctk\inspector\logo.png ..\ctk\emoji\emoji.data) do @call create-lists.bat file resources_sources.mak %f]
!endif

!if [call create-lists.bat footer resources_sources.mak]
!endif

!if [call create-lists.bat header resources_sources.mak CTK_DEMO_RESOURCES]
!endif

!if [for /f %f in ('$(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir=..\demos\ctk-demo ..\demos\ctk-demo\demo.gresource.xml') do @call create-lists.bat file resources_sources.mak %f]
!endif

!if [call create-lists.bat footer resources_sources.mak]
!endif

!if [call create-lists.bat header resources_sources.mak ICON_BROWSER_RESOURCES]
!endif

!if [for /f %f in ('$(GLIB_COMPILE_RESOURCES) --sourcedir=..\demos\icon-browser --generate-dependencies ..\demos\icon-browser\iconbrowser.gresource.xml') do @call create-lists.bat file resources_sources.mak %f]
!endif

!if [call create-lists.bat footer resources_sources.mak]
!endif

!include resources_sources.mak

!if [del /f /q resources_sources.mak]
!endif
