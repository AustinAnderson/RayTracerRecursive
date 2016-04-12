#ifndef SCENE_OBJECT_H
#define SCENE_OBJECT_H
#include "Shape.h"
#include "Cube.h"
#include "Cylinder.h"
#include "Cone.h"
#include "Sphere.h"
#include "SceneParser.h"
#include <fstream>
#include <iostream>
class SceneObject {
public:
	Shape* shape;
	Matrix transform;
	Matrix invTransform;
	SceneMaterial material;
    vector<vector<Point> > textureMap;
	PrimitiveType shapeType;

    Point getMappedPoint(Point isectWorldPoint){
		//texture map not in use
		if (!(material.textureMap->isUsed)){
			return Point(material.cAmbient.r, 
						 material.cAmbient.g,
						 material.cAmbient.b);
		}
		double mapX, mapY;
        int mappedNdxX=0;
        int mappedNdxY=0;
		double repeatX = material.textureMap->repeatU;
		double repeatY = material.textureMap->repeatV;
		Point isectObject = invTransform * isectWorldPoint;
		switch (shapeType){
		case SHAPE_CONE:
			//find point on unit square for a cone
			//intersect is on the cap
			if (isectObject[1] == -0.5){
				mapX = isectObject[0];
				mapY = isectObject[2];
			}
			else{
				double value = isectObject[0] / isectObject[2];
				double arctan = atan(value);
				arctan *= arctan;
				mapY = isectObject[1];
				if (arctan > 0){
					mapX = -arctan / (2 * PI);
				}
				else{
					mapX = 1 - (arctan / (2 * PI));
				}
			}
			break;
		
		case SHAPE_CUBE:
			//find point on unit square for a cube
			//x-coordinate is 0.5
			if (isectObject[0] == 0.5 || isectObject[0] == -0.5){
				mapX = 1 - (isectObject[1] + 0.5);
				mapY = 1 - (isectObject[2] + 0.5);
			}
			//y-coordinate is 0.5
			else if (isectObject[1] == 0.5 || isectObject[1] == -0.5){
				mapX = isectObject[2] + 0.5;
				mapY = isectObject[0] + 0.5;
			}
			//z-coordinate is 0.5
			else{
				mapX = 1 - (isectObject[1] + 0.5);
				mapY = 1 - (isectObject[0] + 0.5);
			}
			break;
		
		case SHAPE_CYLINDER:
			//find point on unit square for a cylinder
			//intersect is on a cap
			if (isectObject[1] == 0.5 || isectObject[1] == -0.5){
				mapX = isectObject[0];
				mapY = isectObject[2];
			}
			else{
				double value = isectObject[0] / isectObject[2];
				double arctan = atan(value);
				arctan *= arctan;
				mapY = isectObject[1];
				if (arctan > 0){
					mapX = -arctan / (2 * PI);
				}
				else{
					mapX = 1 - (arctan / (2 * PI));
				}
			}
			break;
		
		case SHAPE_SPHERE:
			//find point on unit square for a sphere
			mapY = (asin(isectObject[1] / 0.5) / PI) + 0.5;
			//intersect is not at either pole
			if(mapY != 0 && mapY != 1){
				double value = isectObject[0] / isectObject[2];
				double arctan = atan(value);
				arctan *= arctan;
				if (arctan > 0){
					mapX = -arctan / (2 * PI);
				}
				else{
					mapX = 1 - (arctan / (2 * PI));
				}
			}
			//intersect is at a pole
			else{
				mapX = 0.5;
			}
			break;

		default:
			exit(1);
			break;
		}
		//mappedNdxX and mappedNdxY now point to the unit square.
		//Now, we point them to the texture map.
		mappedNdxX = width * mapX;
		mappedNdxY = height * mapY;
		//Now, to map the point so that repeats can occur.
		//mappedNdxX = (mappedNdxX * repeatX);
		//mappedNdxX = mappedNdxX % width;
		//mappedNdxY = mappedNdxY * repeatY;
		//mappedNdxY = mappedNdxY % height;
		if (mappedNdxX >= width){
			mappedNdxX = width - 1;
		}
		if (mappedNdxY >= height){
			mappedNdxY = height - 1;
		}
        return textureMap[mappedNdxX][mappedNdxY];
    }
    void mapTexture(){
        if (material.textureMap->isUsed){
            string line;
            ifstream file;
            file.open(material.textureMap->filename);
            getline(file, line);
            //verify that the first line is "P3"
            if (line != "P3"){
                cout << "ERROR: " << material.textureMap->filename << " is not a valid texture map" << endl;
            }
            else{
                //ignore next line; comment
                getline(file, line);
                //read width and height of texture map
                file >> line;
                width = atoi(line.c_str());
                file >> line;
                height = atoi(line.c_str());
                vector<Point> row;
                for (int i = 0; i < width; i++){
                    for (int j = 0; j < height; j++){
                        Point toAdd;
						file >> line;
						//toAdd[0] = atoi(line.c_str());
						toAdd[0] = atoi(line.c_str()) / 255.0;
                        file >> line;
						//toAdd[1] = atoi(line.c_str());
						toAdd[1] = atoi(line.c_str()) / 255.0;
						file >> line;
						//toAdd[2] = atoi(line.c_str());
						toAdd[2] = atoi(line.c_str()) / 255.0;
                        row.push_back(toAdd);
                    }
                    textureMap.push_back(row);
                    row.clear();
                }
            }
            file.close();
        }
    }
private:
    int width;
    int height;
};
#endif//SCENE_OBJECT_H
