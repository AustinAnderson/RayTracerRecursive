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

    Point getMappedPoint(Point isectWorldPoint){
        int mappedNdxX=0;
        int mappedNdxY=0;
        /*
         * map stuff here
         *
         * using shape transform, etc
         *
         */
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
                        toAdd[0] = atoi(line.c_str());
                        file >> line;
                        toAdd[1] = atoi(line.c_str());
                        file >> line;
                        toAdd[2] = atoi(line.c_str());
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
