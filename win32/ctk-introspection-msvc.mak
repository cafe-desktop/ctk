# NMake Makefile to build Introspection Files for CTK+

!include detectenv-msvc.mak

APIVERSION = 3.0

CHECK_PACKAGE = cdk-pixbuf-2.0 atk pangocairo gio-2.0

built_install_girs =	\
	vs$(VSVER)\$(CFG)\$(PLAT)\bin\Cdk-$(APIVERSION).gir	\
	vs$(VSVER)\$(CFG)\$(PLAT)\bin\CdkWin32-$(APIVERSION).gir	\
	vs$(VSVER)\$(CFG)\$(PLAT)\bin\Ctk-$(APIVERSION).gir

built_install_typelibs =	\
	vs$(VSVER)\$(CFG)\$(PLAT)\bin\Cdk-$(APIVERSION).typelib	\
	vs$(VSVER)\$(CFG)\$(PLAT)\bin\CdkWin32-$(APIVERSION).typelib	\
	vs$(VSVER)\$(CFG)\$(PLAT)\bin\Ctk-$(APIVERSION).typelib

!include introspection-msvc.mak

!if "$(BUILD_INTROSPECTION)" == "TRUE"

!if "$(PLAT)" == "x64"
AT_PLAT=x86_64
!else
AT_PLAT=i686
!endif

all: setgirbuildenv $(built_install_girs) $(built_install_typelibs)

setgirbuildenv:
	@set PYTHONPATH=$(PREFIX)\lib\gobject-introspection
	@set PATH=vs$(VSVER)\$(CFG)\$(PLAT)\bin;$(PREFIX)\bin;$(PATH)
	@set PKG_CONFIG_PATH=$(PKG_CONFIG_PATH)
	@set LIB=vs$(VSVER)\$(CFG)\$(PLAT)\bin;$(LIB)

!include introspection.body.mak

install-introspection: all
	@-copy vs$(VSVER)\$(CFG)\$(PLAT)\bin\*.gir "$(G_IR_INCLUDEDIR)"
	@-copy /b vs$(VSVER)\$(CFG)\$(PLAT)\bin\*.typelib "$(G_IR_TYPELIBDIR)"

!else
all:
	@-echo $(ERROR_MSG)
!endif

clean:
	@-del /f/q vs$(VSVER)\$(CFG)\$(PLAT)\bin\*.typelib
	@-del /f/q vs$(VSVER)\$(CFG)\$(PLAT)\bin\*.gir
