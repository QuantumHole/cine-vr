all:
	mkdir -p build
	g++ -std=c++14 -c $$(pkg-config --cflags openvr) -o build/main.o source/main.cpp
	g++ -std=c++14 $(pkg-config --libs openvr) -lGL -lGLEW -lglfw -lopenvr_api -o cine-vr build/main.o
