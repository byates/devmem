
PROJECT_ROOT := $(abspath $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/.)

# Check that given variables are set and all have non-empty values,
# die with an error otherwise.
#
# Params:
#   1. Variable name(s) to test.
#   2. (optional) Error message to print.
check_defined = \
    $(strip $(foreach 1,$1, \
        $(call __check_defined,$1,$(strip $(value 2)))))
__check_defined = \
    $(if $(value $1),, \
      $(error Undefined $1$(if $2, ($2))))

### APPLICATION ###
EXECUTABLE = devmem
SOURCES := $(wildcard src/*.c)
GDB_SERVER_PORT = "9091"

# REMOTE_IP and REMOTE_KEY environment variables must be defined for the copy 
# part of this make file to work. One _easy_ place to put them is in the 
# VSCode user settings file: ~/.config/Code/User/settings.json
# example:
#     "terminal.integrated.env.linux": {
#        "REMOTE_IP" :  "<user>@<ip>",
#        "REMOTE_KEY" : "<path-to-private-key>"
#    }
$(call check_defined, REMOTE_IP, "REMOTE_IP must be defined for copy to work")
$(call check_defined, REMOTE_KEY, "REMOTE_KEY must be defined for copy to work")

APP_NAME := $(PROJECT_ROOT)/Bin/$(EXECUTABLE)
APP_LIBS :=
APP_BUILD_FLAGS := -std=gnu11 $(LDFLAGS)
APP_COMPILE_FLAGS := -O0 -ggdb -Wall -Wno-unknown-pragmas
APP_INCLUDES := -I$(PROJECT_ROOT)
APP_LIBS_DIR :=

.DEFAULT_GOAL := all

OBJ_DIR = $(PROJECT_ROOT)/Obj/$(EXECUTABLE)
OBJS_FILENAMES := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SOURCES))

# Always rebuild all tests and examples because changes in the libraries are not tracked
.PHONY: $(EXECUTABLE)

# Build all
.PHONY: clean

start: createDirectories
	@echo '==> Building target: $(EXECUTABLE)'

createDirectories:
	@mkdir -p $(PROJECT_ROOT)/Bin
	@mkdir -p $(OBJ_DIR)/src

$(OBJ_DIR)/%.o: %.c
	@echo 'Building file: $<'
	@$(CROSS_COMPILE)gcc $(APP_BUILD_FLAGS) $(APP_INCLUDES) $(APP_COMPILE_FLAGS) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

$(EXECUTABLE): % : start $(OBJS_FILENAMES)
	@$(CROSS_COMPILE)g++ $(APP_BUILD_FLAGS) $(APP_LIBS_DIR) -o $(APP_NAME) $(OBJS_FILENAMES) $(APP_LIBS)
	@echo 'Finished successfully building: "$(EXECUTABLE)"'
	@echo ' '

all: $(EXECUTABLE)

build: clean all

clean-default:
	-@rm -rf $(PROJECT_ROOT)/Bin
	-@rm -rf $(PROJECT_ROOT)/Obj
	@echo 'Clean complete for "$(EXECUTABLE)"'

clean: clean-default

.PHONY: copy_to_remote

copy_to_remote:
	scp -i ${REMOTE_KEY} '${APP_NAME}' ${REMOTE_IP}:~



