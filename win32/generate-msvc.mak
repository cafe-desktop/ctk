# NMake Makefile portion for code generation and
# intermediate build directory creation
# Items in here should not need to be edited unless
# one is maintaining the NMake build files.

!include config-msvc.mak
!include create-lists-msvc.mak

# Copy the pre-defined cdkconfig.h.[win32|win32_broadway]
!if "$(CFG)" == "release" || "$(CFG)" == "Release"
CDK_OLD_CFG = debug
!else
CDK_OLD_CFG = release
!endif

!ifdef BROADWAY
CDK_CONFIG = broadway
CDK_DEL_CONFIG = win32
CDK_CONFIG_TEMPLATE = ..\cdk\cdkconfig.h.win32_broadway
!else
CDK_CONFIG = win32
CDK_DEL_CONFIG = broadway
CDK_CONFIG_TEMPLATE = ..\cdk\cdkconfig.h.win32
!endif

CDK_MARSHALERS_FLAGS = --prefix=_cdk_marshal --valist-marshallers
CDK_RESOURCES_ARGS = ..\cdk\cdk.gresource.xml --target=$@ --sourcedir=..\cdk --c-name _cdk --manual-register
CTK_MARSHALERS_FLAGS = --prefix=_ctk_marshal --valist-marshallers
CTK_RESOURCES_ARGS = ..\ctk\ctk.gresource.xml --target=$@ --sourcedir=..\ctk --c-name _ctk --manual-register

all:	\
	..\config.h	\
	..\cdk\cdkconfig.h	\
	..\cdk\cdkversionmacros.h	\
	..\cdk\cdkmarshalers.h	\
	..\cdk\cdkmarshalers.c	\
	..\cdk\cdkresources.h	\
	..\cdk\cdkresources.c	\
	..\ctk\ctk-win32.rc	\
	..\ctk\libctk3.manifest	\
	..\ctk\ctkdbusgenerated.h	\
	..\ctk\ctkdbusgenerated.c	\
	..\ctk\ctktypefuncs.inc	\
	..\ctk\ctk.gresource.xml	\
	..\ctk\ctkmarshalers.h	\
	..\ctk\ctkmarshalers.c	\
	..\ctk\ctkresources.h	\
	..\ctk\ctkresources.c	\
	..\demos\ctk-demo\demos.h	\
	..\demos\ctk-demo\demo_resources.c	\
	..\demos\icon-browser\resources.c

# Copy the pre-defined config.h.win32 and demos.h.win32
..\config.h: ..\config.h.win32
..\demos\ctk-demo\demos.h: ..\demos\ctk-demo\demos.h.win32
..\ctk\ctk-win32.rc: ..\ctk\ctk-win32.rc.body

..\cdk-$(CFG)-$(CDK_CONFIG)-build: $(CDK_CONFIG_TEMPLATE)
	@if exist ..\cdk-$(CDK_OLD_CFG)-$(CDK_DEL_CONFIG)-build del ..\cdk-$(CDK_OLD_CFG)-$(CDK_DEL_CONFIG)-build
	@if exist ..\cdk-$(CDK_OLD_CFG)-$(CDK_CONFIG)-build del ..\cdk-$(CDK_OLD_CFG)-$(CDK_CONFIG)-build
	@if exist ..\cdk-$(CFG)-$(CDK_DEL_CONFIG)-build del ..\cdk-$(CFG)-$(CDK_DEL_CONFIG)-build
	@copy $** $@
	
..\cdk\cdkconfig.h: ..\cdk-$(CFG)-$(CDK_CONFIG)-build

..\config.h	\
..\cdk\cdkconfig.h	\
..\ctk\ctk-win32.rc	\
..\demos\ctk-demo\demos.h:
	@echo Copying $@...
	@copy $** $@

..\cdk\cdkversionmacros.h: ..\cdk\cdkversionmacros.h.in
	@echo Generating $@...
	@$(PYTHON) gen-cdkversionmacros-h.py --version=$(CTK_VERSION)

..\cdk\cdkmarshalers.h: ..\cdk\cdkmarshalers.list
	@echo Generating $@...
	@$(PYTHON) $(GLIB_GENMARSHAL) $(CDK_MARSHALERS_FLAGS) --header $** > $@.tmp
	@move $@.tmp $@

..\cdk\cdkmarshalers.c: ..\cdk\cdkmarshalers.list
	@echo Generating $@...
	@$(PYTHON) $(GLIB_GENMARSHAL) $(CDK_MARSHALERS_FLAGS) --body $** > $@.tmp
	@move $@.tmp $@

..\cdk\cdk.gresource.xml: $(CDK_RESOURCES)
	@echo Generating $@...
	@echo ^<?xml version='1.0' encoding='UTF-8'?^> >$@
	@echo ^<gresources^> >> $@
	@echo  ^<gresource prefix='/org/ctk/libcdk'^> >> $@
	@for %%f in (..\cdk\resources\glsl\*.glsl) do @echo     ^<file alias='glsl/%%~nxf'^>resources/glsl/%%~nxf^</file^> >> $@
	@echo   ^</gresource^> >> $@
	@echo ^</gresources^> >> $@

..\cdk\cdkresources.h: ..\cdk\cdk.gresource.xml
	@echo Generating $@...
	@if not "$(XMLLINT)" == "" set XMLLINT=$(XMLLINT)
	@if not "$(JSON_GLIB_FORMAT)" == "" set JSON_GLIB_FORMAT=$(JSON_GLIB_FORMAT)
	@if not "$(CDK_PIXBUF_PIXDATA)" == "" set CDK_PIXBUF_PIXDATA=$(CDK_PIXBUF_PIXDATA)
	@start /min $(GLIB_COMPILE_RESOURCES) $(CDK_RESOURCES_ARGS) --generate-header

..\cdk\cdkresources.c: ..\cdk\cdk.gresource.xml $(CDK_RESOURCES)
	@echo Generating $@...
	@if not "$(XMLLINT)" == "" set XMLLINT=$(XMLLINT)
	@if not "$(JSON_GLIB_FORMAT)" == "" set JSON_GLIB_FORMAT=$(JSON_GLIB_FORMAT)
	@if not "$(CDK_PIXBUF_PIXDATA)" == "" set CDK_PIXBUF_PIXDATA=$(CDK_PIXBUF_PIXDATA)
	@start /min $(GLIB_COMPILE_RESOURCES) $(CDK_RESOURCES_ARGS) --generate-source

..\ctk\libctk3.manifest: ..\ctk\libctk3.manifest.in
	@echo Generating $@...
	@$(PYTHON) replace.py	\
	--action=replace-var	\
	--input=$**	--output=$@	\
	--var=EXE_MANIFEST_ARCHITECTURE	\
	--outstring=*

..\ctk\ctkdbusgenerated.h ..\ctk\ctkdbusgenerated.c: ..\ctk\ctkdbusinterfaces.xml
	@echo Generating CTK DBus sources...
	@$(PYTHON) $(PREFIX)\bin\gdbus-codegen	\
	--interface-prefix org.Ctk. --c-namespace _Ctk	\
	--generate-c-code ctkdbusgenerated $**	\
	--output-directory $(@D)

..\ctk\ctktypefuncs.inc: ..\ctk\gentypefuncs.py
	@echo Generating $@...
	@echo #undef CTK_COMPILATION > $(@R).preproc.c
	@echo #include "ctkx.h" >> $(@R).preproc.c
	@cl /EP $(CTK_PREPROCESSOR_FLAGS) $(@R).preproc.c > $(@R).combined.c
	@$(PYTHON) $** $@ $(@R).combined.c
	@del $(@R).preproc.c $(@R).combined.c

..\ctk\ctk.gresource.xml: $(CTK_RESOURCES)
	@echo Generating $@...
	@echo ^<?xml version='1.0' encoding='UTF-8'?^>> $@
	@echo ^<gresources^>>> $@
	@echo   ^<gresource prefix='/org/ctk/libctk'^>>> $@
	@echo     ^<file^>theme/Adwaita/ctk.css^</file^>>> $@
	@echo     ^<file^>theme/Adwaita/ctk-dark.css^</file^>>> $@
	@echo     ^<file^>theme/Adwaita/ctk-contained.css^</file^>>> $@
	@echo     ^<file^>theme/Adwaita/ctk-contained-dark.css^</file^>>> $@
	@for %%f in (..\ctk\theme\Adwaita\assets\*.png) do @echo     ^<file preprocess='to-pixdata'^>theme/Adwaita/assets/%%~nxf^</file^>>> $@
	@for %%f in (..\ctk\theme\Adwaita\assets\*.svg) do @echo     ^<file^>theme/Adwaita/assets/%%~nxf^</file^>>> $@
	@echo     ^<file^>theme/HighContrast/ctk.css^</file^>>> $@
	@echo     ^<file alias='theme/HighContrastInverse/ctk.css'^>theme/HighContrast/ctk-inverse.css^</file^>>> $@
	@echo     ^<file^>theme/HighContrast/ctk-contained.css^</file^>>> $@
	@echo     ^<file^>theme/HighContrast/ctk-contained-inverse.css^</file^>>> $@
	@for %%f in (..\ctk\theme\HighContrast\assets\*.png) do @echo     ^<file preprocess='to-pixdata'^>theme/HighContrast/assets/%%~nxf^</file^>>> $@
	@for %%f in (..\ctk\theme\HighContrast\assets\*.svg) do @echo     ^<file^>theme/HighContrast/assets/%%~nxf^</file^>>> $@
	@echo     ^<file^>theme/win32/ctk-win32-base.css^</file^>>> $@
	@echo     ^<file^>theme/win32/ctk.css^</file^>>> $@
	@for %%f in (..\ctk\cursor\*.png) do @echo     ^<file^>cursor/%%~nxf^</file^>>> $@
	@for %%f in (..\ctk\gesture\*.symbolic.png) do @echo     ^<file alias='icons/64x64/actions/%%~nxf'^>gesture/%%~nxf^</file^>>> $@
	@for %%f in (..\ctk\ui\*.ui) do @echo     ^<file preprocess='xml-stripblanks'^>ui/%%~nxf^</file^>>> $@
	@for %%s in (16 22 24 32 48) do @(for %%c in (actions status categories) do @(for %%f in (..\ctk\icons\%%sx%%s\%%c\*.png) do @echo     ^<file^>icons/%%sx%%s/%%c/%%~nxf^</file^>>> $@))
	@for %%s in (scalable) do @(for %%c in (status) do @(for %%f in (..\ctk\icons\%%s\%%c\*.svg) do @echo     ^<file^>icons/%%s/%%c/%%~nxf^</file^>>> $@))
	@for %%f in (..\ctk\inspector\*.ui) do @echo     ^<file compressed='true' preprocess='xml-stripblanks'^>inspector/%%~nxf^</file^>>> $@
	@echo     ^<file^>inspector/logo.png^</file^>>> $@
	@echo     ^<file^>emoji/emoji.data^</file^>>> $@
	@echo   ^</gresource^>>> $@
	@echo ^</gresources^>>> $@

..\ctk\ctkresources.h: ..\ctk\ctk.gresource.xml
	@echo Generating $@...
	@if not "$(XMLLINT)" == "" set XMLLINT=$(XMLLINT)
	@if not "$(JSON_GLIB_FORMAT)" == "" set JSON_GLIB_FORMAT=$(JSON_GLIB_FORMAT)
	@if not "$(CDK_PIXBUF_PIXDATA)" == "" set CDK_PIXBUF_PIXDATA=$(CDK_PIXBUF_PIXDATA)
	@start /min $(GLIB_COMPILE_RESOURCES) $(CTK_RESOURCES_ARGS) --generate-header

..\ctk\ctkresources.c: ..\ctk\ctk.gresource.xml $(CTK_RESOURCES)
	@echo Generating $@...
	@if not "$(XMLLINT)" == "" set XMLLINT=$(XMLLINT)
	@if not "$(JSON_GLIB_FORMAT)" == "" set JSON_GLIB_FORMAT=$(JSON_GLIB_FORMAT)
	@if not "$(CDK_PIXBUF_PIXDATA)" == "" set CDK_PIXBUF_PIXDATA=$(CDK_PIXBUF_PIXDATA)
	@start /min $(GLIB_COMPILE_RESOURCES) $(CTK_RESOURCES_ARGS) --generate-source

..\ctk\ctkmarshalers.h: ..\ctk\ctkmarshalers.list
	@echo Generating $@...
	@$(PYTHON) $(GLIB_GENMARSHAL) $(CTK_MARSHALERS_FLAGS) --header $** > $@.tmp
	@move $@.tmp $@

..\ctk\ctkmarshalers.c: ..\ctk\ctkmarshalers.list
	@echo Generating $@...
	@echo #undef G_ENABLE_DEBUG> $@.tmp
	@$(PYTHON) $(GLIB_GENMARSHAL) $(CTK_MARSHALERS_FLAGS) --body $** >> $@.tmp
	@move $@.tmp $@

..\demos\ctk-demo\demo_resources.c: ..\demos\ctk-demo\demo.gresource.xml $(CTK_DEMO_RESOURCES)
	@echo Generating $@...
	@$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(@D) --generate-source $(@D)\demo.gresource.xml

..\demos\icon-browser\resources.c: ..\demos\icon-browser\iconbrowser.gresource.xml $(ICON_BROWSER_RESOURCES)
	@echo Generating $@...
	@$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(@D) --generate-source $(@D)\iconbrowser.gresource.xml

# Remove the generated files
clean:
	@-del /f /q ..\demos\icon-browser\resources.c
	@-del /f /q ..\demos\ctk-demo\demo_resources.c
	@-del /f /q ..\demos\ctk-demo\demos.h
	@-del /f /q ..\ctk\ctkresources.c
	@-del /f /q ..\ctk\ctkresources.h
	@-del /f /q ..\ctk\ctkmarshalers.c
	@-del /f /q ..\ctk\ctkmarshalers.h
	@-del /f /q ..\ctk\ctk.gresource.xml
	@-del /f /q ..\ctk\ctktypefuncs.inc
	@-del /f /q ..\ctk\ctkdbusgenerated.c
	@-del /f /q ..\ctk\ctkdbusgenerated.h
	@-del /f /q ..\ctk\libctk3.manifest
	@-del /f /q ..\ctk\ctk-win32.rc
	@-del /f /q ..\cdk\cdkresources.c
	@-del /f /q ..\cdk\cdkresources.h
	@-del /f /q ..\cdk\cdk.gresource.xml
	@-del /f /q ..\cdk\cdkmarshalers.c
	@-del /f /q ..\cdk\cdkmarshalers.h
	@-del /f /q ..\cdk\cdkversionmacros.h
	@-del /f /q ..\cdk\cdkconfig.h
	@if exist ..\cdk-$(CFG)-$(CDK_CONFIG)-build del ..\cdk-$(CFG)-$(CDK_CONFIG)-build
	@if exist ..\cdk-$(CDK_OLD_CFG)-$(CDK_DEL_CONFIG)-build del ..\cdk-$(CDK_OLD_CFG)-$(CDK_DEL_CONFIG)-build
	@if exist ..\cdk-$(CDK_OLD_CFG)-$(CDK_CONFIG)-build del ..\cdk-$(CDK_OLD_CFG)-$(CDK_CONFIG)-build
	@if exist ..\cdk-$(CFG)-$(CDK_DEL_CONFIG)-build del ..\cdk-$(CFG)-$(CDK_DEL_CONFIG)-build
	@-del /f /q ..\config.h
