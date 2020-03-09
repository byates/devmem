
PROJECT_ROOT := $(abspath $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/.)

### APPLICATION ###
EXECUTABLE = devmem
SOURCES := $(wildcard *.c)

APP_NAME := $(PROJECT_ROOT)/Bin/$(EXECUTABLE)
APP_LIBS :=
APP_BUILD_FLAGS := -std=gnu11 $(LDFLAGS)
APP_COMPILE_FLAGS := -O0 -g -Wall -Wno-unknown-pragmas
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
	@mkdir -p $(OBJ_DIR)

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


