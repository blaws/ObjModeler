GCC_OPTIONS=-Wall -pedantic -Iinclude -Iinclude/tinyobjloader -g -Wno-deprecated
GL_OPTIONS=-framework OpenGL -framework GLUT -framework CoreFoundation
OPTIONS=$(GCC_OPTIONS) $(GL_OPTIONS)

all: modeler

objParser.o: objParser.cpp
	g++ -c $^ $(GCC_OPTIONS)

InitShader.o: include/InitShader.cpp
	g++ -c $^ $(GCC_OPTIONS)

trackball.o: include/trackball.c
	g++ -c $^ $(GCC_OPTIONS)

tiny_obj_loader.o: include/tinyobjloader/tiny_obj_loader.cc
	g++ -c $^ $(GCC_OPTIONS)

modeler.o: modeler.cpp
	g++ -c $^ $(GCC_OPTIONS)

modeler: modeler.o InitShader.o trackball.o tiny_obj_loader.o objParser.o include/libSOIL.a
	g++ $^ $(OPTIONS) -o $@  

clean:
	rm -rf *.o *~ *.dSYM modeler
