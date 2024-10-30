#
# Base makefile for Linux.
#
# !!!!! Note to future editors !!!!!
# 
# before you make changes, make sure you grok:
# 1. the difference between =, :=, +=, and ?= 
# 2. how and when this base makefile gets included in the generated makefile(s)
#  ( see http://www.gnu.org/software/make/manual/make.html#Flavors )
#
# Command line prefixes:
#  -	errors are ignored
#  @	command is not printed to stdout before being executed
#  +	command is executed even if Make is invoked in "do not exec" mode

OS := $(shell uname)
HOSTNAME := $(shell hostname)

-include $(SRCROOT)/devtools/steam_def.mak
-include $(SRCROOT)/devtools/sourcesdk_def.mak

# setaf 1 Red
# setaf 2 Green
# setaf 3 Yellow
# setaf 4 Blue
# setaf 5 Purple
# setaf 6 Cyan
# setaf 7 Gray

RED     = $(shell tput setaf 1)
GREEN   = $(shell tput setaf 2)
YELLOW  = $(shell tput setaf 3)
BLUE    = $(shell tput setaf 4)
PURPLE  = $(shell tput setaf 5)
CYAN    = $(shell tput setaf 6)
DEFAULT = $(shell tput sgr0)

# To build with clang, set the following in your environment:
#   CC = clang
#   CXX = clang++
ifneq (,$(findstring clang,$(CXX)))
	CLANG_BUILD = 1
endif

ifeq ($(OS),Darwin)
    $(error This file should never be used for Mac - use base.xconfig)
endif

ifeq ($(CLANG_BUILD),1)
	# i haven't touched clang so idk
	BASE_CFLAGS += #
else
	# gcc - work better w/ preprocessing shennanigans (cbase etc), and track vars better for easier dbging
	# i think fvar-tracking-assignments might also resolve some weird quirk with bad code gen??
	# fsani / stackreuse also do the same wrt codegen, probably? maybe?
	# BASE_CFLAGS += -fpch-preprocess -fvar-tracking-assignments -fsanitize=undefined -fstack-reuse=none
endif

ifeq ($(CFG), release)
	# With gcc 4.6.3, engine.so went from 7,383,765 to 8,429,109 when building with -O3.
	#  There also was no speed difference running at 1280x1024. May 2012, mikesart.
	#  tonyp: The size increase was likely caused by -finline-functions and -fipa-cp-clone getting switched on with -O3.
	#
	# -O3 causes funny issues (gcc9.2) with unclickable buttons on main menu!
	# specifying -finline-functions / -finline-small-functions / -findirect-inlining manually seems to
	# cause funny issues with compiling random funcs, throwing an error w/ Wformat & having a null second param (despite it never being null)
	# so i guess if you're going to start somewhere trying to get O3 working, start by doing -fno-inline-funcs etc ?
	OptimizerLevel_CompilerSpecific  = -O2
	# fix unclickable buttons (?!?!)
        OptimizerLevel_CompilerSpecific += -fno-inline-functions -fno-inline-small-functions -fno-indirect-inlining
	# needed for forcing us to always have a frame ptr (eip)
	OptimizerLevel_CompilerSpecific += -fno-omit-frame-pointer
	# if i dont specify this gcc(9.2) produces BADC0DE in CItemEffectMeterManager; e.g.
	# m_Meters[i]->Update( pPlayer );
	# m_pProgressBar->SetProgress( flProgress );
	# etc.
	OptimizerLevel_CompilerSpecific += -fno-tree-dse
	# if i dont specify this gcc(9.2) produces BADC0DE in KeyValues::RemoveEverything
	OptimizerLevel_CompilerSpecific += -fno-delete-null-pointer-checks
	# if i dont specify this gcc(9.1) produces BADC0DE in CBaseEntityOutput::FireOutput
        OptimizerLevel_CompilerSpecific += -fno-partial-inlining
        OptimizerLevel_CompilerSpecific += -fno-schedule-insns2
	# cherry pick some good optimizations (this was here before i got here)
        OptimizerLevel_CompilerSpecific += -ffast-math -ftree-vectorize
	# compiler will WHINE if we dont specify this
        OptimizerLevel_CompilerSpecific += -fno-strict-aliasing
	# This whole section was last touched by sappho sept/oct/nov 2022, pester me if you need to

	ifeq ($(CLANG_BUILD),1)
		# These aren't supported with Clang 3.5. Need to remove when we update that.
		# OptimizerLevel_CompilerSpecific += ?
	else
		OptimizerLevel_CompilerSpecific += -fpredictive-commoning -funswitch-loops
	endif
else
	OptimizerLevel_CompilerSpecific = -Og
	#-O1 -finline-functions
endif

# CPPFLAGS == "c/c++ *preprocessor* flags" - not "cee-plus-plus flags"
ARCH_FLAGS = 
BUILDING_MULTI_ARCH = 0
# Preserve cflags set in environment
ENV_CFLAGS := $(CFLAGS)
ENV_CXXFLAGS := $(CXXFLAGS)
CPPFLAGS = $(DEFINES) $(addprefix -I, $(abspath $(INCLUDEDIRS) ))
BASE_CFLAGS = $(ARCH_FLAGS) $(CPPFLAGS) $(WARN_FLAGS) $(HARDEN_CFLAGS) -fvisibility=$(SymbolVisibility) $(OptimizerLevel) -pipe $(GCC_ExtraCompilerFlags) -Usprintf -Ustrncpy -UPROTECTED_THINGS_ENABLE
CFLAGS = $(BASE_CFLAGS) $(ENV_CFLAGS) -fasynchronous-unwind-tables # <- helps w/ debugging -sappho
# In -std=gnu++0x mode we get lots of errors about "error: narrowing conversion". -fpermissive
# turns these into warnings in gcc, and -Wno-c++11-narrowing suppresses them entirely in clang 3.1+.
ifeq ($(CLANG_BUILD),1)
	CXXFLAGS = $(BASE_CFLAGS) -std=gnu++17 -Wno-c++11-narrowing -Wno-dangling-else $(ENV_CXXFLAGS)
else
	CXXFLAGS = $(BASE_CFLAGS) -std=gnu++17 -fpermissive $(ENV_CXXFLAGS)
	# https://gcc.gnu.org/onlinedocs/gcc/C_002b_002b-Dialect-Options.html - 0 matches the c++ spec as closely as possible
	# CXXFLAGS += -fabi-version=2
	# ensure that GCC's name mangling will be compatible with the binary blobs from Valve that were compiled with potentially older GCC versions
	CXXFLAGS += -fabi-compat-version=2
	# -Wabi
endif

DEFINES += -DVPROF_LEVEL=1 -DGNUC -DNO_HOOK_MALLOC -DNO_MALLOC_OVERRIDE

## TODO: This cases build errors in cstrike/bin right now. Need to debug.
# This causes all filesystem interfaces to default to their 64bit versions on
# 32bit systems, which means we don't break on filesystems with inodes > 32bit.
# DEFINES += -D_FILE_OFFSET_BITS=64

LDFLAGS = $(CFLAGS) $(GCC_ExtraLinkerFlags) $(OptimizerLevel) $(HARDEN_LFLAGS) -pthread -lstdc++fs -ggdb -g
GENDEP_CXXFLAGS = -MMD -MP -MF $(@:.o=.P) 
MAP_FLAGS =
Srv_GAMEOUTPUTFILE = 
COPY_DLL_TO_SRV = 0

# We should always specify -Wl,--build-id, as documented at:
# http://linux.die.net/man/1/ld and http://fedoraproject.org/wiki/Releases/FeatureBuildId.http://fedoraproject.org/wiki/Releases/FeatureBuildId
LDFLAGS += -Wl,--build-id

#
# If we should be running in a chroot, check to see if we are. If not, then prefix everything with the 
# required chroot
#
ifdef MAKE_CHROOT
	export STEAM_RUNTIME_PATH := /usr
	ifneq ("$(SCHROOT_CHROOT_NAME)", "$(CHROOT_NAME)")
        $(info '$(SCHROOT_CHROOT_NAME)' is not '$(CHROOT_NAME)')
        $(error This makefile should be run from within a chroot. 'schroot --chroot $(CHROOT_NAME) -- $(MAKE) $(MAKEFLAGS)')  
	endif
	GCC_VER = -9
	P4BIN = $(SRCROOT)/devtools/bin/linux/p4
	CRYPTOPPDIR=ubuntu12_32_gcc48
else ifeq ($(USE_VALVE_BINDIR),1)
	# Using /valve/bin directory.
	export STEAM_RUNTIME_PATH ?= /valve
	GCC_VER = -9
	P4BIN = p4
	CRYPTOPPDIR=linux32
else
	# Not using chroot, use old steam-runtime. (gcc 4.6.3)
	export STEAM_RUNTIME_PATH ?= /valve/steam-runtime
	GCC_VER =
	P4BIN = p4
	CRYPTOPPDIR=ubuntu12_32
endif

ifeq ($(TARGET_PLATFORM),linux64)
	MARCH_TARGET = core2
else
	MARCH_TARGET = pentium4
endif

ifeq ($(USE_VALVE_BINDIR),1)
	# On dedicated servers, some plugins depend on global variable symbols in addition to functions.
	# So symbols like _Z16ClearMultiDamagev should show up when you do "nm server_srv.so" in TF2.
	STRIP_FLAGS =
else
	# Linux desktop client (or client/dedicated server in chroot).
	STRIP_FLAGS = -x
endif

CCACHE := $(SRCROOT)/devtools/bin/linux/ccache

ifeq ($(origin AR), default)
	AR = $(STEAM_RUNTIME_PATH)/bin/ar crs
endif
ifeq ($(origin CC), default)
	CC = $(CCACHE) $(STEAM_RUNTIME_PATH)/bin/gcc$(GCC_VER)	
endif
ifeq ($(origin CXX), default)
	CXX = $(CCACHE) $(STEAM_RUNTIME_PATH)/bin/g++$(GCC_VER)
endif

# Support ccache with clang. Add -Qunused-arguments to avoid excessive warnings due to
# a ccache quirk. Could also upgrade ccache.
# http://petereisentraut.blogspot.com/2011/05/ccache-and-clang.html
ifeq ($(CLANG_BUILD),1)
	CC := $(CCACHE) $(CC) -Qunused-arguments -fcolor-diagnostics
	CXX := $(CCACHE) $(CXX) -Qunused-arguments -fcolor-diagnostics
endif
LINK ?= $(CC)

ifeq ($(STEAM_BRANCH),1)
	WARN_FLAGS = -Wall -Wextra -Wshadow -Wno-invalid-offsetof
else ifeq ($(CFG),release)
	WARN_FLAGS = -Wall -Wno-invalid-offsetof -Wno-multichar -Wno-overloaded-virtual
	WARN_FLAGS += -Wno-write-strings
	WARN_FLAGS += -Wno-unused-variable
	WARN_FLAGS += -Wno-unused-but-set-variable
	WARN_FLAGS += -Wno-unused-function
  # m_pMemory = (T*)realloc( m_pMemory, m_nAllocationCount * sizeof(T) );
  WARN_FLAGS += -Wno-class-memaccess
  # DEFINITELY fixme!
  WARN_FLAGS += -Wno-narrowing
  # fixme!
  WARN_FLAGS += -Wno-ignored-attributes
	# fixme!
  WARN_FLAGS += -Wno-unused-local-typedefs
  # for ignoring assert compares
  WARN_FLAG  += -Wno-nonnull-compare
else
    WARN_FLAGS = -Wall -Wno-address
  # m_pMemory = (T*)realloc( m_pMemory, m_nAllocationCount * sizeof(T) );
  WARN_FLAGS += -Wno-class-memaccess
  # DEFINITELY fixme!
  WARN_FLAGS += -Wno-narrowing
  # fixme!
  WARN_FLAGS += -Wno-ignored-attributes
  # fixme!
  WARN_FLAGS += -Wno-unused-local-typedefs
  # for ignoring assert compares
  WARN_FLAGS += -Wno-nonnull-compare
endif


ifeq ($(CLANG_BUILD),1)
	# Clang specific flags
else ifeq ($(GCC_VER),-4.8)
	WARN_FLAGS += -Wno-unused-local-typedefs
	WARN_FLAGS += -Wno-unused-result
	WARN_FLAGS += -Wno-narrowing
	# WARN_FLAGS += -Wno-unused-function
endif

WARN_FLAGS += -Wno-unknown-pragmas -Wno-unused-parameter -Wno-unused-value -Wno-missing-field-initializers
WARN_FLAGS += -Wno-sign-compare -Wno-reorder -Wno-invalid-offsetof -Wno-float-equal -Werror=return-type
WARN_FLAGS += -fdiagnostics-show-option -Wformat -Wformat-security



# Hardening! -sappho

# https://developers.redhat.com/blog/2018/03/21/compiler-and-linker-flags-gcc
# Hardening#non-exec_memory_segmentation_.28ExecShield.29
# etc

# Compile-time protection against static sized buffer overflows. No known regressions or performance loss. This should be enabled system-wide
HARDEN_CFLAGS      += -D_FORTIFY_SOURCE=2
# PIE = Position Independent Executable = https://en.wikipedia.org/wiki/Position-independent_code#Position-independent_executables, needed to take advantage of ASLR
# -fPIC = position independent code
#    When generating code for shared libraries, -fpic implies -msmall-data and -fPIC implies -mlarge-data.
#    Large data == up to ~2GB data area, small data == 64kb, which is definitely not enough
HARDEN_CFLAGS      += -fPIE -fPIC
# Werror=format-security makes the build FAIL if a format string is insecure/prone to buff ovflow
# Werror=format is similar
HARDEN_CFLAGS      += -Wformat -Wformat-security -Werror=format -Werror=format-security
# -fstack-protector-strong prevents stack fuckery by implementing stack canaries on functions that "probably need it";
#    local variable’s address used as part of the right hand side of an assignment or function argument
#    local variable is an array (or union containing an array), regardless of array type or length
#    uses register local variables
# HARDEN_CFLAGS      += -fstack-protector-strong
# fstack-clash-protection means "the compiler will only allocate one page of stack space at a time and each page is accessed immediately after allocation". recommended by redhat
# fasynchronous-unwind-tables means the unwind table generated is precisely at an instruction boundary, enabling accurate unwinding at any instruction; helps debugging / backtracing
HARDEN_CFLAGS      += -fstack-clash-protection
# D_GLIBCXX_ASSERTIONS forces run time bounds checking for c++ strs/vecs
HARDEN_CFLAGS      += -D_GLIBCXX_ASSERTIONS

# no lazy binding ; resolve symbols on program start instead of "as needed"
HARDEN_LFLAGS      += -Wl,-z,now
# read only relo segments ; https://www.redhat.com/en/blog/hardening-elf-binaries-using-relocation-read-only-relro
HARDEN_LFLAGS      += -Wl,-z,relro
# no underlinking ; force all symbols to be defined
HARDEN_LFLAGS      += -Wl,-z,defs



MARCH_TARGET  = core2
MTUNE_TARGET  = generic

# ifeq ($(CLANG_BUILD),1)
# 	# Clang does not support -mfpmath=sse because it uses whatever
# 	# instruction set extensions are available by default.
# 	SSE_GEN_FLAGS = -msse3
# else
# 	SSE_GEN_FLAGS = -msse3 -mfpmath=sse
# endif

ifeq ($(TARGET_PLATFORM),linux64)
	# nocona = pentium4 + 64bit + MMX, SSE, SSE2, SSE3 - no SSSE3 (that's three s's - added in core2)
	ARCH_FLAGS += -march=$(MARCH_TARGET) -mtune=$(MTUNE_TARGET)
	ARCH_FLAGS += -fdiagnostics-color=always
	LD_SO = ld-linux-x86_64.so.2
	LIBSTDCXX := $(shell $(CXX) -print-file-name=libstdc++.a)
	LIBSTDCXXPIC := $(shell $(CXX) -print-file-name=libstdc++-pic.a)
else
	# pentium4 = MMX, SSE, SSE2 - no SSE3 (added in prescott) # -msse3 -mfpmath=sse
	ARCH_FLAGS += -m32 -march=$(MARCH_TARGET) -mtune=$(MTUNE_TARGET)
	ARCH_FLAGS += -fdiagnostics-color=always
	LD_SO = ld-linux.so.2
	LIBSTDCXX := $(shell $(CXX) $(ARCH_FLAGS) -print-file-name=libstdc++.a)
	LIBSTDCXXPIC := $(shell $(CXX) $(ARCH_FLAGS) -print-file-name=libstdc++.a)
	LDFLAGS += -m32
endif

GEN_SYM ?= $(SRCROOT)/devtools/gendbg.sh
ifeq ($(CFG),release)
	STRIP ?= strip $(STRIP_FLAGS) -S
#	CFLAGS += -ffunction-sections -fdata-sections
#	LDFLAGS += -Wl,--gc-sections -Wl,--print-gc-sections
else
	STRIP ?= true
endif
VSIGN ?= true

# Don't need to do this, you only need server_srv, and that needs to be a symlink, as well
# Also this is so sloppily done that it's making "srv" versions of client files. Yuck!
# -sappho
# ifeq ($(SOURCE_SDK), 1)
# 	Srv_GAMEOUTPUTFILE := $(GAMEOUTPUTFILE:.so=_srv.so)
# 	COPY_DLL_TO_SRV := 1
# endif

LINK_MAP_FLAGS = -Wl,-Map,$(@:.so=).map

SHLIBLDFLAGS = -shared $(LDFLAGS) -Wl,--no-undefined

_WRAP := -Xlinker --wrap=
PATHWRAP = $(_WRAP)fopen $(_WRAP)freopen $(_WRAP)open    $(_WRAP)creat    $(_WRAP)access  $(_WRAP)__xstat \
	   $(_WRAP)stat  $(_WRAP)lstat   $(_WRAP)fopen64 $(_WRAP)open64   $(_WRAP)opendir $(_WRAP)__lxstat \
	   $(_WRAP)chmod $(_WRAP)chown   $(_WRAP)lchown  $(_WRAP)symlink  $(_WRAP)link    $(_WRAP)__lxstat64 \
	   $(_WRAP)mknod $(_WRAP)utimes  $(_WRAP)unlink  $(_WRAP)rename   $(_WRAP)utime   $(_WRAP)__xstat64 \
	   $(_WRAP)mount $(_WRAP)mkfifo  $(_WRAP)mkdir   $(_WRAP)rmdir    $(_WRAP)scandir $(_WRAP)realpath

LIB_START_EXE = $(PATHWRAP) -static-libgcc -Wl,--start-group
LIB_END_EXE = -Wl,--end-group -lm -ldl $(LIBSTDCXX) -lpthread 

LIB_START_SHLIB = $(PATHWRAP) -static-libgcc -Wl,--start-group
LIB_END_SHLIB = -Wl,--end-group -lm -ldl $(LIBSTDCXXPIC) -lpthread -l:$(LD_SO) -Wl,--version-script=$(SRCROOT)/devtools/version_script.linux.txt

#
# Profile-directed optimizations.
# Note: Last time these were tested 3/5/08, it actually slowed down the server benchmark by 5%!
#
# First, uncomment these, build, and test. It will generate .gcda and .gcno files where the .o files are.
# PROFILE_LINKER_FLAG=-fprofile-arcs
# PROFILE_COMPILER_FLAG=-fprofile-arcs
#
# Then, comment the above flags out again and rebuild with this flag uncommented:
# PROFILE_COMPILER_FLAG=-fprofile-use
#

#############################################################################
# The compiler command lne for each src code file to compile
#############################################################################

OBJ_DIR = ./obj_$(NAME)_$(TARGET_PLATFORM)$(TARGET_PLATFORM_EXT)/$(CFG)
CPP_TO_OBJ = $(CPPFILES:.cpp=.o)
CXX_TO_OBJ = $(CPP_TO_OBJ:.cxx=.o)
CC_TO_OBJ = $(CXX_TO_OBJ:.cc=.o)
MM_TO_OBJ = $(CC_TO_OBJ:.mm=.o)
C_TO_OBJ = $(MM_TO_OBJ:.c=.o)
OBJS = $(addprefix $(OBJ_DIR)/, $(notdir $(C_TO_OBJ)))

ifeq ($(MAKE_VERBOSE),1)
	QUIET_PREFIX = 
	QUIET_ECHO_POSTFIX = 
else
	QUIET_PREFIX = @
	QUIET_ECHO_POSTFIX = > /dev/null
endif

ifeq ($(MAKE_CC_VERBOSE),1)
CC += -v
endif

ifeq ($(CONFTYPE),lib)
  LIB_File = $(OUTPUTFILE)
endif

ifeq ($(CONFTYPE),dll)
  SO_File = $(OUTPUTFILE)
endif

ifeq ($(CONFTYPE),exe)
  EXE_File = $(OUTPUTFILE)
endif

# we generate dependencies as a side-effect of compilation now
GEN_DEP_FILE=

PRE_COMPILE_FILE = 

POST_COMPILE_FILE = 

ifeq ($(BUILDING_MULTI_ARCH),1)
	SINGLE_ARCH_CXXFLAGS=$(subst -arch x86_64,,$(CXXFLAGS))
	COMPILE_FILE = \
		$(QUIET_PREFIX) \
		echo "${DEFAULT}---> $(lastword $(subst /, ,$<))${DEFAULT} as MULTIARCH${PURPLE}"; \
		mkdir -p $(OBJ_DIR) && \
		$(CXX) $(SINGLE_ARCH_CXXFLAGS) $(GENDEP_CXXFLAGS) -o $@ -c $< && \
		$(CXX) $(CXXFLAGS) -o $@ -c $<
else
	COMPILE_FILE = \
		$(QUIET_PREFIX) \
		echo "${DEFAULT}${CYAN}[`basename $(OUTPUTFILE)`]\t${GREEN}$(lastword $(subst /, ,$<))${PURPLE}"; \
		mkdir -p $(OBJ_DIR) && \
		$(CXX) $(CXXFLAGS) $(GENDEP_CXXFLAGS) -o $@ -c $<
endif

ifneq "$(origin VALVE_NO_AUTO_P4)" "undefined"
	P4_EDIT_START = true #chmod -R +w
	P4_EDIT_END = #|| true
	P4_REVERT_START = true
	P4_REVERT_END =
else
	ifndef P4_EDIT_CHANGELIST
		# You can use an environment variable to specify what changelist to check the Linux Binaries out into. Normally the default
		# setting is best, but here is an alternate example:
		# export P4_EDIT_CHANGELIST_CMD="echo 1424335"
		# ?= means that if P4_EDIT_CHANGELIST_CMD is already set it won't be changed.
		P4_EDIT_CHANGELIST_CMD ?= $(P4BIN) changes -c `$(P4BIN) client -o | grep ^Client | cut -f 2` -s pending | fgrep 'POSIX Auto Checkout' | cut -d' ' -f 2 | tail -n 1
		P4_EDIT_CHANGELIST := $(shell $(P4_EDIT_CHANGELIST_CMD))
	endif
	ifeq ($(P4_EDIT_CHANGELIST),)
		# If we haven't found a changelist to check out to then create one. The name must match the one from a few
		# lines above or else a new changelist will be created each time.
		# Warning: the behavior of 'echo' is not consistent. In bash you need the "-e" option in order for \n to be
		# interpreted as a line-feed, but in dash you do not, and if "-e" is passed along then it is printed, which
		# confuses p4. So, if you run this command from the bash shell don't forget to add "-e" to the echo command.
		P4_EDIT_CHANGELIST = $(shell echo -e "Change: new\nDescription: POSIX Auto Checkout" | $(P4BIN) change -i | cut -f 2 -d ' ')
	endif

	P4_EDIT_START := for f in
	P4_EDIT_END := ; do if [ -n $$f ]; then if [ -d $$f ]; then find $$f -type f -print | $(P4BIN) -x - edit -c $(P4_EDIT_CHANGELIST); else $(P4BIN) edit -c $(P4_EDIT_CHANGELIST) $$f; fi; fi; done $(QUIET_ECHO_POSTFIX)
	P4_REVERT_START := for f in  
	P4_REVERT_END := ; do if [ -n $$f ]; then if [ -d $$f ]; then find $$f -type f -print | $(P4BIN) -x - revert; else $(P4BIN) revert $$f; fi; fi; done $(QUIET_ECHO_POSTFIX) 
endif

ifeq ($(CONFTYPE),dll)
all: $(OTHER_DEPENDENCIES) $(OBJS) $(GAMEOUTPUTFILE)
	@echo $(GAMEOUTPUTFILE) $(QUIET_ECHO_POSTFIX)
else
all: $(OTHER_DEPENDENCIES) $(OBJS) $(OUTPUTFILE)
	@echo $(OUTPUTFILE) $(QUIET_ECHO_POSTFIX)
endif

.PHONY: clean cleantargets cleanandremove rebuild relink RemoveOutputFile SingleFile


rebuild :
	$(MAKE) -f $(firstword $(MAKEFILE_LIST)) cleanandremove
	$(MAKE) -f $(firstword $(MAKEFILE_LIST))


# Use the relink target to force to relink the project.
relink: RemoveOutputFile all

RemoveOutputFile:
	rm -f $(OUTPUTFILE)


# This rule is so you can say "make SingleFile SingleFilename=/home/myname/valve_main/src/engine/language.cpp" and have it only build that file.
# It basically just translates the full filename to create a dependency on the appropriate .o file so it'll build that.
SingleFile : RemoveSingleFile $(OBJ_DIR)/$(basename $(notdir $(SingleFilename))).o
	@echo ""

RemoveSingleFile:
	$(QUIET_PREFIX) rm -f $(OBJ_DIR)/$(basename $(notdir $(SingleFilename))).o

clean:
ifneq "$(OBJ_DIR)" ""
	$(QUIET_PREFIX) echo "rm -rf $(OBJ_DIR)"
	$(QUIET_PREFIX) rm -rf $(OBJ_DIR)
endif
ifneq "$(OUTPUTFILE)" ""
	$(QUIET_PREFIX) if [ -e $(OUTPUTFILE) ]; then \
		echo "$(P4BIN) revert $(OUTPUTFILE)"; \
		$(P4_REVERT_START) $(OUTPUTFILE) $(OUTPUTFILE)$(SYM_EXT) $(P4_REVERT_END); \
	fi;
endif
ifneq "$(OTHER_DEPENDENCIES)" ""
	$(QUIET_PREFIX) echo "rm -f $(OTHER_DEPENDENCIES)"
	$(QUIET_PREFIX) rm -f $(OTHER_DEPENDENCIES)
endif
ifneq "$(GAMEOUTPUTFILE)" ""
	$(QUIET_PREFIX) echo "$(P4BIN) revert $(GAMEOUTPUTFILE)"
	$(QUIET_PREFIX) $(P4_REVERT_START) $(GAMEOUTPUTFILE) $(GAMEOUTPUTFILE)$(SYM_EXT) $(P4_REVERT_END)
endif


# Do the above cleaning, except with p4 edit and rm. Reason being ar crs adds and replaces obj files to the
# archive. However if you've renamed or deleted a source file, $(AR) won't remove it. This can leave
# us with archive files that have extra unused symbols, and also potentially cause compilation errors
# when you rename a file and have many duplicate symbols.
cleanandremove:
ifneq "$(OBJ_DIR)" ""
	$(QUIET_PREFIX) echo "rm -rf $(OBJ_DIR)"
	$(QUIET_PREFIX) -rm -rf $(OBJ_DIR)
endif
ifneq "$(OUTPUTFILE)" ""
	$(QUIET_PREFIX) if [ -e $(OUTPUTFILE) ]; then \
		echo "$(P4BIN) edit and rm -f $(OUTPUTFILE) $(OUTPUTFILE)$(SYM_EXT)"; \
		$(P4_EDIT_START) $(OUTPUTFILE) $(OUTPUTFILE)$(SYM_EXT) $(P4_EDIT_END); \
	fi;
	$(QUIET_PREFIX) -rm -f $(OUTPUTFILE) $(OUTPUTFILE)$(SYM_EXT);
endif
ifneq "$(OTHER_DEPENDENCIES)" ""
	$(QUIET_PREFIX) echo "rm -f $(OTHER_DEPENDENCIES)"
	$(QUIET_PREFIX) -rm -f $(OTHER_DEPENDENCIES)
endif
ifneq "$(GAMEOUTPUTFILE)" ""
	$(QUIET_PREFIX) echo "$(P4BIN) edit and rm -f $(GAMEOUTPUTFILE) $(GAMEOUTPUTFILE)$(SYM_EXT)"
	$(QUIET_PREFIX) $(P4_EDIT_START) $(GAMEOUTPUTFILE) $(GAMEOUTPUTFILE)$(SYM_EXT) $(P4_EDIT_END)
	$(QUIET_PREFIX) -rm -f $(GAMEOUTPUTFILE)
endif


# This just deletes the final targets so it'll do a relink next time we build.
cleantargets:
	$(QUIET_PREFIX) rm -f $(OUTPUTFILE) $(GAMEOUTPUTFILE)


$(LIB_File): $(OTHER_DEPENDENCIES) $(OBJS) 
	$(QUIET_PREFIX) -$(P4_EDIT_START) $(LIB_File) $(P4_EDIT_END); 
	$(QUIET_PREFIX) $(AR) $(LIB_File) $(OBJS) $(LIBFILES);

SO_GameOutputFile = $(GAMEOUTPUTFILE)

# Remove the target before installing a file over it; this prevents existing
# instances of the game from crashing due to the overwrite.
$(SO_GameOutputFile): $(SO_File)
	$(QUIET_PREFIX) \
	$(P4_EDIT_START) $(GAMEOUTPUTFILE) $(P4_EDIT_END) && \
	echo "----" $(QUIET_ECHO_POSTFIX);\
	echo "---- COPYING TO $@ [$(CFG)] ----";\
	echo "----" $(QUIET_ECHO_POSTFIX);
	$(QUIET_PREFIX) -$(P4_EDIT_START) $(GAMEOUTPUTFILE) $(P4_EDIT_END);
	$(QUIET_PREFIX) -mkdir -p `dirname $(GAMEOUTPUTFILE)` > /dev/null;
	$(QUIET_PREFIX) rm -f $(GAMEOUTPUTFILE) $(QUIET_ECHO_POSTFIX);
	$(QUIET_PREFIX) cp -v $(OUTPUTFILE) $(GAMEOUTPUTFILE) $(QUIET_ECHO_POSTFIX);
	$(QUIET_PREFIX) -$(P4_EDIT_START) $(GAMEOUTPUTFILE)$(SYM_EXT) $(P4_EDIT_END);
	$(QUIET_PREFIX) $(GEN_SYM) $(GAMEOUTPUTFILE); 
	$(QUIET_PREFIX) -$(STRIP) $(GAMEOUTPUTFILE);
	$(QUIET_PREFIX) $(VSIGN) -signvalve $(GAMEOUTPUTFILE);
	$(QUIET_PREFIX) if [ "$(COPY_DLL_TO_SRV)" = "1" ]; then\
		echo "----" $(QUIET_ECHO_POSTFIX);\
		echo "---- COPYING TO $(Srv_GAMEOUTPUTFILE) ----";\
		echo "----" $(QUIET_ECHO_POSTFIX);\
		cp -v $(GAMEOUTPUTFILE) $(Srv_GAMEOUTPUTFILE) $(QUIET_ECHO_POSTFIX);\
		cp -v $(GAMEOUTPUTFILE)$(SYM_EXT) $(Srv_GAMEOUTPUTFILE)$(SYM_EXT) $(QUIET_ECHO_POSTFIX);\
	fi;
	$(QUIET_PREFIX) if [ "$(IMPORTLIBRARY)" != "" ]; then\
		echo "----" $(QUIET_ECHO_POSTFIX);\
		echo "---- COPYING TO IMPORT LIBRARY $(IMPORTLIBRARY) ----";\
		echo "----" $(QUIET_ECHO_POSTFIX);\
		$(P4_EDIT_START) $(IMPORTLIBRARY) $(P4_EDIT_END) && \
		mkdir -p `dirname $(IMPORTLIBRARY)` > /dev/null && \
		cp -v $(OUTPUTFILE) $(IMPORTLIBRARY); \
	fi;


$(SO_File): $(OTHER_DEPENDENCIES) $(OBJS) $(LIBFILENAMES)
	$(QUIET_PREFIX) \
	echo "----" $(QUIET_ECHO_POSTFIX);\
	echo "---- LINKING $@ [$(CFG)] ----";\
	echo "----" $(QUIET_ECHO_POSTFIX);\
	\
	$(LINK) $(LINK_MAP_FLAGS) $(SHLIBLDFLAGS) $(PROFILE_LINKER_FLAG) -o $(OUTPUTFILE) $(LIB_START_SHLIB) $(OBJS) $(LIBFILES) $(SystemLibraries) $(LIB_END_SHLIB);
	$(VSIGN) -signvalve $(OUTPUTFILE);


$(EXE_File) : $(OTHER_DEPENDENCIES) $(OBJS) $(LIBFILENAMES)
	$(QUIET_PREFIX) \
	echo "----" $(QUIET_ECHO_POSTFIX);\
	echo "---- LINKING EXE $@ [$(CFG)] ----";\
	echo "----" $(QUIET_ECHO_POSTFIX);\
	\
	$(P4_EDIT_START) $(OUTPUTFILE) $(P4_EDIT_END);\
	$(LINK) $(LINK_MAP_FLAGS) $(LDFLAGS) $(PROFILE_LINKER_FLAG) -o $(OUTPUTFILE) $(LIB_START_EXE) $(OBJS) $(LIBFILES) $(SystemLibraries) $(LIB_END_EXE);
	$(QUIET_PREFIX) -$(P4_EDIT_START) $(OUTPUTFILE)$(SYM_EXT) $(P4_EDIT_END);
	$(QUIET_PREFIX) $(GEN_SYM) $(OUTPUTFILE);
	$(QUIET_PREFIX) -$(STRIP) $(OUTPUTFILE);
	$(QUIET_PREFIX) $(VSIGN) -signvalve $(OUTPUTFILE);


tags:
	etags -a -C -o $(SRCROOT)/TAGS *.cpp *.cxx *.h *.hxx