all: Assignment5.o Camera.o SceneParser.o tinystr.o tinyxml.o tinyxmlerror.o tinyxmlparser.o
		g++ Assignment5.o Camera.o SceneParser.o tinystr.o tinyxml.o tinyxmlerror.o tinyxmlparser.o \
				-lglui -lglut -lGL -lGLU
Assignment5.o: Assignment5.cpp SceneObject.h Shape.h Algebra.h Cube.h \
 Cylinder.h Cone.h Sphere.h SceneParser.h SceneData.h Camera.h
		g++ -c -g -o Assignment5.o Assignment5.cpp
Camera.o: Camera.cpp Camera.h Algebra.h
		g++ -c -g -o Camera.o Camera.cpp 
SceneParser.o: SceneParser.cpp SceneParser.h SceneData.h Algebra.h \
 tinyxml.h tinystr.h
		g++ -c -g -o SceneParser.o SceneParser.cpp 
tinystr.o: tinystr.cpp tinystr.h
		g++ -c -g -o tinystr.o tinystr.cpp 
tinyxml.o: tinyxml.cpp tinyxml.h
		g++ -c -g -o tinyxml.o tinyxml.cpp 
tinyxmlerror.o: tinyxmlerror.cpp tinyxml.h
		g++ -c -g -o tinyxmlerror.o tinyxmlerror.cpp 
tinyxmlparser.o: tinyxmlparser.cpp tinyxml.h
		g++ -c -g -o tinyxmlparser.o tinyxmlparser.cpp 
clean:
		rm *.o
