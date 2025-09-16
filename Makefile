# SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later

PROJECT_ROOT := .
DEPENDENCIES = "glfw3 openvr glew"

include common/cplusplus.mk
include common/license.mk
include common/makefile.mk

TARGET = cine-vr

OBJECTS = \
	$(BUILD_DIR)/main.o \
	$(BUILD_DIR)/shader_set.o \
	$(BUILD_DIR)/openvr_interface.o \
	$(BUILD_DIR)/framebuffer.o \
	$(BUILD_DIR)/enum_iterator.o \
    $(BUILD_DIR)/ebo.o \
    $(BUILD_DIR)/vao.o \
    $(BUILD_DIR)/vbo.o \
    $(BUILD_DIR)/vertex.o \
    $(BUILD_DIR)/shape.o \

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "[$(LD)] $@"
	@$(CXX) $(LDFLAGS) -o $@ $^ -s $(LDLIBS)

lint:
	$(MAKE) format
	-$(MAKE) license-annotate
	$(MAKE) license-lint

kill:
	killall -9 vrserver vrcompositor vrdashboard $(TARGET)
