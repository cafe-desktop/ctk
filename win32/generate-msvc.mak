# NMake Makefile portion for code generation and
# intermediate build directory creation
# Items in here should not need to be edited unless
# one is maintaining the NMake build files.

!include config-msvc.mak
!include create-lists-msvc.mak

# Copy the pre-defined gdkconfig.h.[win32|win32_broadway]
!if "$(CFG)" == "release" || "$(CFG)" == "Release"
GDK_OLD_CFG = debug
!else
GDK_OLD_CFG = release
!endif

!ifdef BROADWAY
GDK_CONFIG = broadway
GDK_DEL_CONFIG = win32
GDK_CONFIG_TEMPLATE = ..\gdk\gdkconfig.h.win32_broadway
!else
GDK_CONFIG = win32
GDK_DEL_CONFIG = broadway
GDK_CONFIG_TEMPLATE = ..\gdk\gdkconfig.h.win32
!endif

GDK_MARSHALERS_FLAGS = --prefix=_gdk_marshal --valist-marshallers
GDK_RESOURCES_ARGS = ..\gdk\gdk.gresource.xml --target=$@ --sourcedir=..\gdk --c-name _gdk --manual-register
CTK_MARSHALERS_FLAGS = --prefix=_ctk_marshal --valist-marshallers
CTK_RESOURCES_ARGS = ..\ctk\ctk.gresource.xml --target=$@ --sourcedir=..\ctk --c-name _ctk --manual-register

all:	\
	..\config.h	\
	..\gdk\gdkconfig.h	\
	..\gdk\gdkversionmacros.h	\
	..\gdk\gdkmarshalers.h	\
	..\gdk\gdkmarshalers.c	\
	..\gdk\gdkresources.h	\
	..\gdk\gdkresources.c	\
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

..\gdk-$(CFG)-$(GDK_CONFIG)-build: $(GDK_CONFIG_TEMPLATE)
	@if exist ..\gdk-$(GDK_OLD_CFG)-$(GDK_DEL_CONFIG)-build del ..\gdk-$(GDK_OLD_CFG)-$(GDK_DEL_CONFIG)-build
	@if exist ..\gdk-$(GDK_OLD_CFG)-$(GDK_CONFIG)-build del ..\gdk-$(GDK_OLD_CFG)-$(GDK_CONFIG)-build
	@if exist ..\gdk-$(CFG)-$(GDK_DEL_CONFIG)-build del ..\gdk-$(CFG)-$(GDK_DEL_CONFIG)-build
	@copy $** $@
	
..\gdk\gdkconfig.h: ..\gdk-$(CFG)-$(GDK_CONFIG)-build

..\config.h	\
..\gdk\gdkconfig.h	\
..\ctk\ctk-win32.rc	\
..\demos\ctk-demo\demos.h:
	@echo Copying $@...
	@copy $** $@

..\gdk\gdkversionmacros.h: ..\gdk\gdkversionmacros.h.in
	@echo Generating $@...
	@$(PYTHON) gen-gdkversionmacros-h.py --version=$(CTK_VERSION)

..\gdk\gdkmarshalers.h: ..\gdk\gdkmarshalers.list
	@echo Generating $@...
	@$(PYTHON) $(GLIB_GENMARSHAL) $(GDK_MARSHALERS_FLAGS) --header $** > $@.tmp
	@move $@.tmp $@

..\gdk\gdkmarshalers.c: ..\gdk\gdkmarshalers.list
	@echo Generating $@...
	@$(PYTHON) $(GLIB_GENMARSHAL) $(GDK_MARSHALERS_FLAGS) --body $** > $@.tmp
	@move $@.tmp $@

..\gdk\gdk.gresource.xml: $(GDK_RESOURCES)
	@echo Generating $@...
	@echo ^<?xml version='1.0' encoding='UTF-8'?^> >$@
	@echo ^<gresources^> >> $@
	@echo  ^<gresource prefix='/org/ctk/libgdk'^> >> $@
	@for %%f in (..\gdk\resources\glsl\*.glsl) do @echo     ^<file alias='glsl/%%~nxf'^>resources/glsl/%%~nxf^</file^> >> $@
	@echo   ^</gresource^> >> $@
	@echo ^</gresources^> >> $@

..\gdk\gdkresources.h: ..\gdk\gdk.gresource.xml
	@echo Generating $@...
	@if not "$(XMLLINT)" == "" set XMLLINT=$(XMLLINT)
	@if not "$(JSON_GLIB_FORMAT)" == "" set JSON_GLIB_FORMAT=$(JSON_GLIB_FORMAT)
	@if not "$(GDK_PIXBUF_PIXDATA)" == "" set GDK_PIXBUF_PIXDATA=$(GDK_PIXBUF_PIXDATA)
	@start /min $(GLIB_COMPILE_RESOURCES) $(GDK_RESOURCES_ARGS) --generate-header

..\gdk\gdkresources.c: ..\gdk\gdk.gresource.xml $(GDK_RESOURCES)
	@echo Generating $@...
	@if not "$(XMLLINT)" == "" set XMLLINT=$(XMLLINT)
	@if not "$(JSON_GLIB_FORMAT)" == "" set JSON_GLIB_FORMAT=$(JSON_GLIB_FORMAT)
	@if not "$(GDK_PIXBUF_PIXDATA)" == "" set GDK_PIXBUF_PIXDATA=$(GDK_PIXBUF_PIXDATA)
	@start /min $(GLIB_COMPILE_RESOURCES) $(GDK_RESOURCES_ARGS) --generate-source

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
	@if not "$(GDK_PIXBUF_PIXDATA)" == "" set GDK_PIXBUF_PIXDATA=$(GDK_PIXBUF_PIXDATA)
	@start /min $(GLIB_COMPILE_RESOURCES) $(CTK_RESOURCES_ARGS) --generate-header

..\ctk\ctkresources.c: ..\ctk\ctk.gresource.xml $(CTK_RESOURCES)
	@echo Generating $@...
	@if not "$(XMLLINT)" == "" set XMLLINT=$(XMLLINT)
	@if not "$(JSON_GLIB_FORMAT)" == "" set JSON_GLIB_FORMAT=$(JSON_GLIB_FORMAT)
	@if not "$(GDK_PIXBUF_PIXDATA)" == "" set GDK_PIXBUF_PIXDATA=$(GDK_PIXBUF_PIXDATA)
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
	@-del /f /q ..\gdk\gdkresources.c
	@-del /f /q ..\gdk\gdkresources.h
	@-del /f /q ..\gdk\gdk.gresource.xml
	@-del /f /q ..\gdk\gdkmarshalers.c
	@-del /f /q ..\gdk\gdkmarshalers.h
	@-del /f /q ..\gdk\gdkversionmacros.h
	@-del /f /q ..\gdk\gdkconfig.h
	@if exist ..\gdk-$(CFG)-$(GDK_CONFIG)-build del ..\gdk-$(CFG)-$(GDK_CONFIG)-build
	@if exist ..\gdk-$(GDK_OLD_CFG)-$(GDK_DEL_CONFIG)-build del ..\gdk-$(GDK_OLD_CFG)-$(GDK_DEL_CONFIG)-build
	@if exist ..\gdk-$(GDK_OLD_CFG)-$(GDK_CONFIG)-build del ..\gdk-$(GDK_OLD_CFG)-$(GDK_CONFIG)-build
	@if exist ..\gdk-$(CFG)-$(GDK_DEL_CONFIG)-build del ..\gdk-$(CFG)-$(GDK_DEL_CONFIG)-build
	@-del /f /q ..\config.h
