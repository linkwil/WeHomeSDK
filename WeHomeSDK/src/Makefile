
# define the exlude dirs here if need
exclude_dirs= 

# get all of the sub dirs
dirs:=$(shell find . -maxdepth 1 -type d)
dirs:=$(basename $(patsubst ./%,%,$(dirs)))
dirs:=$(filter-out $(exclude_dirs),$(dirs))
SUBDIRS := $(dirs)


# target source
CSRCSS  := $(wildcard *.c)
CPPSRCS:= $(wildcard *.cpp)
COBJS  := $(CSRCSS:%.c=%.o)
CPPOBJS := $(CPPSRCS:%.cpp=%.o)

.PHONY : all

all: $(COBJS) $(CPPOBJS) subdirs

subdirs:$(SUBDIRS)
	@for dir in $(SUBDIRS);\
		do $(MAKE) -C $$dir all||exit 1;\
	done

$(COBJS):%.o:%.c 
	@echo CC $(<F)
	@$(CC) $(CFLAGS) -c $< -o $(OBJS_DIR)/$@
	
$(CPPOBJS):%.o:%.cpp 
	@echo CXX $(<F)
	@$(CXX) $(CPPFLAGS) -c $< -o $(OBJS_DIR)/$@

