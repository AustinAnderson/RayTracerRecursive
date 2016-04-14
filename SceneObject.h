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
    vector<vector<Point> >* textureMap;
	PrimitiveType shapeType;

    Point getMappedPoint(Point isectWorldPoint){
		//texture map not in use
		if (!(material.textureMap->isUsed)){
			return Point(material.cDiffuse.r, 
						 material.cDiffuse.g,
						 material.cDiffuse.b);
		}
		double mapX, mapY;
        int mappedNdxX=0;
        int mappedNdxY=0;
		double repeatX = material.textureMap->repeatU;
		double repeatY = material.textureMap->repeatV;
		Point isectObject = invTransform * isectWorldPoint;
        const int x=0;//for easier reading when doing points
        const int y=1;
        const int z=2;
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
				if (arctan < 0){
					mapX = -arctan / (2 * PI);
				}
				else{
					mapX = 1 - (arctan / (2 * PI));
				}
			}
			break;
		
		case SHAPE_CUBE:
			//find point on unit square for a cube
			//mapping onto the yz plane
			if (isClose(isectObject[x],0.5) || isClose(isectObject[x],-0.5)){
				mapX = 1 - (isectObject[z] + 0.5);
				mapY = 1 - (isectObject[y] + 0.5);
			}
			// mapping onto the xz plane
			else if (isClose(isectObject[y],0.5) || isClose(isectObject[y],-0.5)){
				mapX = isectObject[z] + 0.5;
				mapY = isectObject[x] + 0.5;
			}
			//mapping onto the xy plane
			else{
				mapX = 1 - (isectObject[y] + 0.5);
				mapY = 1 - (isectObject[x] + 0.5);
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
				if (arctan < 0){
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
				if (arctan < 0){
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
		mappedNdxX = (int)(width * mapX);
		mappedNdxY = (int)(height * mapY);
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
        return (*textureMap)[mappedNdxX][mappedNdxY];
    }
    void mapTexture(){
        if (material.textureMap->isUsed){
            string line;
            ifstream file;
            file.open(material.textureMap->filename.c_str());
			if (!file.is_open()){
				cout << "ERROR: File Not Found" << endl;
				return;
			}
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
				file >> line;
				maxVal = atof(line.c_str());
                vector<Point> row;
                for (int i = 0; i < width; i++){
                    for (int j = 0; j < height; j++){
                        Point toAdd;
						file >> line;
						//toAdd[0] = atoi(line.c_str());
						toAdd[0] = atoi(line.c_str()) / maxVal;
                        file >> line;
						//toAdd[1] = atoi(line.c_str());
						toAdd[1] = atoi(line.c_str()) / maxVal;
						file >> line;
						//toAdd[2] = atoi(line.c_str());
						toAdd[2] = atoi(line.c_str()) / maxVal;
                        row.push_back(toAdd);
                    }
                    textureMap->push_back(row);
                    row.clear();
                }
            }
            file.close();
        }
		return;
    }
private:
    bool isClose(double value1,double value2){
        double result=(value1-value2);
        if(result<0){
            result=result*-1;
        }
        return result<.00001;
    }
    double width;
    double height;
	double maxVal;
};
#endif//SCENE_OBJECT_H
