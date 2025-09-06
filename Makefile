# SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later

PROJECT_ROOT := .
DEPENDENCIES = "glfw3 openvr glew"

include common/cplusplus.mk
include common/license.mk

TARGET = cine-vr

OBJECTS = \
	$(BUILD_DIR)/main.o \

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "[$(LD)] $@"
	@$(CXX) $(LDFLAGS) -o $@ $^ -s $(LDLIBS)

kill:
	killall -9 vrserver vrcompositor vrdashboard $(TARGET)
