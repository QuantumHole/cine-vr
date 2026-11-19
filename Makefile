# SPDX-FileCopyrightText: 2025, 2026 QuantumHole <QuantumHole@github.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later

PROJECT_ROOT := .
DEPENDENCIES := "glfw3 >= 3.4 openvr libpng libjpeg freetype2 mpv gl libavformat libavcodec libavutil libswscale libcrypto sqlite3 fontconfig"

include common/cplusplus.mk
include common/license.mk
include common/makefile.mk
include common/doxygen.mk

TARGET := cine-vr

OBJECTS := \
	$(BUILD_DIR)/main.o \
	$(BUILD_DIR)/application.o \
	$(BUILD_DIR)/shader_set.o \
	$(BUILD_DIR)/openvr_interface.o \
	$(BUILD_DIR)/framebuffer.o \
	$(BUILD_DIR)/enum_iterator.o \
	$(BUILD_DIR)/ebo.o \
	$(BUILD_DIR)/vao.o \
	$(BUILD_DIR)/vbo.o \
	$(BUILD_DIR)/vertex.o \
	$(BUILD_DIR)/shape.o \
	$(BUILD_DIR)/simple_button.o \
	$(BUILD_DIR)/toggle_button.o \
	$(BUILD_DIR)/slide_button.o \
	$(BUILD_DIR)/controller.o \
	$(BUILD_DIR)/texture.o \
	$(BUILD_DIR)/image_data.o \
	$(BUILD_DIR)/render_model.o \
	$(BUILD_DIR)/projection.o \
	$(BUILD_DIR)/menu.o \
	$(BUILD_DIR)/panel.o \
	$(BUILD_DIR)/line_panel.o \
	$(BUILD_DIR)/font_renderer.o \
	$(BUILD_DIR)/file_system.o \
	$(BUILD_DIR)/scroll_panel.o \
	$(BUILD_DIR)/player.o \
	$(BUILD_DIR)/progress_bar.o \
	$(BUILD_DIR)/node_xml.o \
	$(BUILD_DIR)/node_css.o \
	$(BUILD_DIR)/shape_set.o \
	$(BUILD_DIR)/graphics_svg.o \
	$(BUILD_DIR)/string_tools.o \
	$(BUILD_DIR)/render_manager.o \
	$(BUILD_DIR)/sqlitedb.o \
	$(BUILD_DIR)/thumbnail_generator.o \
	$(BUILD_DIR)/thumbnail_store.o \
	$(BUILD_DIR)/list_element.o \
	$(BUILD_DIR)/resource_manager.o \

OBJCOPY := objcopy
RESOURCES := $(shell find images shaders -type f -and \( -name '*.glsl' -or -name '*.svg' \))
RESOURCES_INC := build/resources.inc
RESOURCES_MAP_INC := build/resources_map.inc
RESOURCE_OBJECTS := $(patsubst %,$(BUILD_DIR)/%.o,$(RESOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS) $(RESOURCE_OBJECTS)
	@echo "[$(LD)] $@"
	@$(CXX) $(LDFLAGS) -o $@ $^ -s $(LDLIBS)

$(RESOURCES_INC) $(RESOURCES_MAP_INC): $(RESOURCE_OBJECTS)

$(BUILD_DIR)/%.o: % 
	@mkdir -p $(dir $@)
	@$(OBJCOPY) \
	    -I binary \
	    -O elf64-x86-64 \
	    -B i386:x86-64 \
	    --rename-section .data=.rodata,alloc,load,readonly,data,contents \
	    $< $@
	@SYMBOL=$$(echo '$<' | sed 's/[/. -]/_/g'); \
	    echo "extern \"C\" const char _binary_$${SYMBOL}_start[];" >> $(RESOURCES_INC); \
	    echo "extern \"C\" const char _binary_$${SYMBOL}_end[];" >> $(RESOURCES_INC); \
	    echo "add(\"$*\", _binary_$${SYMBOL}_start, _binary_$${SYMBOL}_end);" >> $(RESOURCES_MAP_INC);

$(BUILD_DIR)/resource_manager.o: $(RESOURCES_INC) $(RESOURCES_MAP_INC) source/util/resource_manager.h source/util/resource_manager.cpp

lint:
	$(MAKE) format
	-$(MAKE) license-annotate
	$(MAKE) license-lint

doc: doxygen-doc
	$(MAKE) doxygen-show

compile:
	clear && $(MAKE) clean && $(MAKE) -j gnu && $(MAKE) clean && $(MAKE) -j llvm

run: compile
	./$(TARGET)

kill:
	killall -9 vrserver vrcompositor vrdashboard $(TARGET)
