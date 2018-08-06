##############################################
## EasyCamSdk Makefile system
##############################################

export TOP_DIR=$(shell pwd)

# define the exlude dirs here if need
exclude_dirs= Build dep include
exclude_files = 

# Add all the include dirs here
INCLUDES := -I$(TOP_DIR)/src \
	-I$(TOP_DIR)/dep/include \
	-I$(TOP_DIR)/include \
	-I$(TOP_DIR)/src/ \
	-I$(TOP_DIR)/src/platform \
	-I$(TOP_DIR)/src/audio_process \

CFLAGS += $(INCLUDES)
CPPFLAGS += $(INCLUDES)

CFLAGS += -Wall -O3 -fPIC
CPPFLAGS += -Wall -O3 -fPIC

# Add all predefine here
CFLAGS += -DLINUX -DWEBRTC_POSIX
CPPFLAGS += -DLINUX -DWEBRTC_POSIX

export CFLAGS
export CPPFLAGS


LDLIB_PATHS := -L$(TOP_DIR)/dep/lib/linux_x64
LDLIBS := -lLINK_API -lPPCS_API -lNDT_API_PPCS

CreateDir        =   $(shell [ -d $1 ] || mkdir -p $1 || echo ":mkdir '$1' fail")
RemoveDir        =   $(shell [ -d $1 ] && rm -rf $1 && echo -e "rmdir '$1'\t [ OK ]" || echo ":rm dir '$1' fail")

dummy :=  $(call CreateDir, $(OBJS_DIR))
dummy :=  $(call CreateDir, $(LIBS_DIR))

# get all of the sub dirs
dirs:=$(shell find . -maxdepth 1 -type d)
dirs:=$(basename $(patsubst ./%,%,$(dirs)))
dirs:=$(filter-out $(exclude_dirs),$(dirs))
SUBDIRS := $(dirs)

# target source
SRCS  := $(wildcard *.c) 
CPPSRCS := $(wildcard *.cpp) 
OBJS  := $(SRCS:%.c=%.o)
CPPOBJS  := $(CPPSRCS:%.c=%.o)

.PHONY : all

all: $(OBJS) $(CPPOBJS) subdirs
	g++ -shared -fpic -o $(LIBS_DIR)/libEasyCamSdk.so $(OBJS_DIR)/*.o $(LDLIBS) $(LDLIB_PATHS)
	@echo "Build complete"

clean:
	@$(RM) -rf $(TOP_DIR)/Output

subdirs:$(SUBDIRS)
	@for dir in $(SUBDIRS);\
		do $(MAKE) -C $$dir all||exit 1;\
	done

$(OBJS):%.o:%.c
	@echo CC $(<F)
	@$(CC) $(CFLAGS) -c $< -o $(OBJS_DIR)/$@
	
$(CPPOBJS):%.o:%.cpp
	@echo CXX $(<F)
	@$(CXX) $(CPPFLAGS) -c $< -o $(OBJS_DIR)/$@



