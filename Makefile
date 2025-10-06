# SPDX-FileCopyrightText: 2025 QuantumHole <QuantumHole@github.com>
#
# SPDX-License-Identifier: GPL-3.0-or-later

PROJECT_ROOT := .
DEPENDENCIES = "glfw3 openvr glew libpng libjpeg"

include common/cplusplus.mk
include common/license.mk
include common/makefile.mk

TARGET = cine-vr
IMAGE_DIR = images

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
	$(BUILD_DIR)/button.o \
	$(BUILD_DIR)/controller.o \
	$(BUILD_DIR)/id.o \
	$(BUILD_DIR)/texture.o \
	$(BUILD_DIR)/image_data.o \
	$(BUILD_DIR)/render_model.o \
	$(BUILD_DIR)/projection.o \
	$(BUILD_DIR)/menu.o \

IMAGES = \
	$(IMAGE_DIR)/angle.png \
	$(IMAGE_DIR)/backward.png \
	$(IMAGE_DIR)/cube-mono.png \
	$(IMAGE_DIR)/cube-stereo.png \
	$(IMAGE_DIR)/cylinder.png \
	$(IMAGE_DIR)/delete.png \
	$(IMAGE_DIR)/fisheye.png \
	$(IMAGE_DIR)/flat.png \
	$(IMAGE_DIR)/forward.png \
	$(IMAGE_DIR)/left-right.png \
	$(IMAGE_DIR)/mono.png \
	$(IMAGE_DIR)/next.png \
	$(IMAGE_DIR)/open.png \
	$(IMAGE_DIR)/pause.png \
	$(IMAGE_DIR)/play.png \
	$(IMAGE_DIR)/power.png \
	$(IMAGE_DIR)/previous.png \
	$(IMAGE_DIR)/sphere.png \
	$(IMAGE_DIR)/top-bottom.png \

all: $(TARGET) $(IMAGES)

$(TARGET): $(OBJECTS)
	@echo "[$(LD)] $@"
	@$(CXX) $(LDFLAGS) -o $@ $^ -s $(LDLIBS)

lint:
	$(MAKE) format
	-$(MAKE) license-annotate
	$(MAKE) license-lint

run:
	clear && $(MAKE) clean && $(MAKE) -j gnu && $(MAKE) clean && $(MAKE) -j llvm && ./$(TARGET)

kill:
	killall -9 vrserver vrcompositor vrdashboard $(TARGET)

clean-images:
	rm -f $(IMAGE_DIR)/*.png

$(IMAGE_DIR)/%.png: $(IMAGE_DIR)/%.svg
	@echo "[magick] $@"
	magick -density $$(echo "0.115234375 * 64" | bc -l) -background none $< $@
	@# density of 0.115234375 leads to images of 1x1 pixels
