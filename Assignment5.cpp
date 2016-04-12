#define NUM_OPENGL_LIGHTS 8

//#include <string.h>
#include <ctime>
#include <pthread.h>
#include <string>
#include <GL/glui.h>
#include "SceneObject.h"
#include "Camera.h"

using namespace std;
Vector getReflectedRay(Vector ray,Vector normal);
int getClosestObjectNdx(Vector ray,Point start,double& minDist);
ostream& operator<<(ostream& os,Point& p){
    os<<"{"<<p[0]<<","<<p[1]<<","<<p[2]<<"}";
    return os;
}


/** These are the live variables passed into GLUI ***/
int  isectOnly = 0;

int  specularOn   = 1;
int  ambientOn    = 1;
int  diffuseOn    = 1;
int  transparentOn= 1;

int	 maxDepth= 1;

int	 camRotU = 0;
int	 camRotV = 0;
int	 camRotW = 0;
int  viewAngle = 45;
float eyeX = 2;
float eyeY = 2;
float eyeZ = 2;
float lookX = -2;
float lookY = -2;
float lookZ = -2;

/** These are GLUI control panel objects ***/
int  main_window;
//string filenamePath = "data/tests/work.xml";
string filenamePath = "data/tests/mirror_test.xml";
GLUI_EditText* filenameTextField = NULL;
GLubyte* pixels = NULL;
int pixelWidth = 0, pixelHeight = 0;
int screenWidth = 0, screenHeight = 0;

std::vector<SceneObject> sceneObjects;

/** these are the global variables used for rendering **/
Cube* cube = new Cube();
Cylinder* cylinder = new Cylinder();
Cone* cone = new Cone();
Sphere* sphere = new Sphere();
SceneParser* parser = NULL;
Camera* camera = new Camera();

void setupCamera();
void updateCamera();
void flattenScene(SceneNode* node, Matrix compositeMatrix);

Vector generateRay(int x, int y){
	double px = -1.0 + 2.0*x/ (double)screenWidth;
	double py = -1.0 + 2.0*y/ (double)screenHeight;
	Point camSreenPoint(px, py, -1);
	//Matrix worldToCamera = camera->GetScaleMatrix() * camera->GetModelViewMatrix();
	//Matrix cameraToWorld = invert(worldToCamera);
	Matrix cameraToWorld = camera->getCamera2WorldMatrix();
	Point worldScreenPoint = cameraToWorld * camSreenPoint;
	Vector ray = worldScreenPoint - camera->GetEyePoint();
	ray.normalize();
	return ray;
}
Vector getReflectedRay(Vector ray,Vector normal){
	double dot_rn = dot(ray, normal);
    return (ray-(2 * dot_rn*normal));
}

void setpixel(GLubyte* buf, int x, int y, int r, int g, int b) {
	buf[(y*pixelWidth + x) * 3 + 0] = (GLubyte)r;
	buf[(y*pixelWidth + x) * 3 + 1] = (GLubyte)g;
	buf[(y*pixelWidth + x) * 3 + 2] = (GLubyte)b;
}

Point calculateColor(SceneObject closestObject, Vector normalVector, Vector ray, Point isectWorldPoint,int recurseDepth) {
    Point Od(closestObject.material.cDiffuse.r *diffuseOn,
             closestObject.material.cDiffuse.g *diffuseOn,
             closestObject.material.cDiffuse.b *diffuseOn); 

    Point Os(closestObject.material.cSpecular.r*specularOn,
             closestObject.material.cSpecular.g*specularOn,
             closestObject.material.cSpecular.b*specularOn);

    Point Or(closestObject.material.cReflective.r*specularOn,
             closestObject.material.cReflective.g*specularOn,
             closestObject.material.cReflective.b*specularOn);

    Point Oa(closestObject.material.cAmbient.r *ambientOn,
             closestObject.material.cAmbient.g *ambientOn,
             closestObject.material.cAmbient.b *ambientOn); 
	Point color;
    double blend = closestObject.material.blend;
    double r_blend = 1 - blend;
	int numLights = parser->getNumLights();
	for (int i = 0; i < numLights; i++) {
		SceneLightData lightData;
		parser->getLightData(i, lightData);

		Vector lightDir = lightData.pos - isectWorldPoint;
        if(lightData.type==LIGHT_DIRECTIONAL){
		    lightDir = -lightData.dir;
        }
		lightDir.normalize();
			
		double dot_nl = dot(normalVector, lightDir);
		double dot_rv = dot(ray, (lightDir-(2 * dot_nl*normalVector)));

		if (dot_nl<0) dot_nl = 0;
		if (dot_rv<0) dot_rv = 0;

		//                      (r.v )^f
		double RdotVToTheF= pow(dot_rv, closestObject.material.shininess);

		Point lightColor(lightData.color.r,
                         lightData.color.g,
                         lightData.color.b);

        double minDist = MIN_ISECT_DISTANCE;//K values multiplied into O's by flatten
        if(getClosestObjectNdx(lightDir,isectWorldPoint,minDist)<0){//if intersection from object to light then shadow, so don't render
            for (int j = 0; j<3; j++) {
                color[j] += 
                        /* 
                          attenuation*
                        //*/
                        lightColor[j]*
                        ((Od[j]* dot_nl)+      //diffuse
                         (Os[j]*RdotVToTheF)); //specular
                        
            }
        }

	}
    //now color is the sum of diffuse and specular times attenuatiion
    //times light color over all lights
    for (int j = 0; j<3; j++) {
        color[j]+=
                /*
                  Ia*
                //*/
                  Oa[j];
        color[j]=(color[j]*blend)+(r_blend*closestObject.getMappedPoint(isectWorldPoint)[j]);//weighted average of calculated color and texture map color
        
        if (color[j]>1) {color[j] = 1.0;}
    }
    //alright time to recurse (*shudders*)
    Point reflectedColor(0,0,0);
    if(recurseDepth>0){
        double minDist=MIN_ISECT_DISTANCE;//record before this to sum them up to calculate attenuation
        Vector reflectedRay=getReflectedRay(ray,normalVector);//RECURSION_WORK
        int closestObjectNdx=getClosestObjectNdx(reflectedRay,isectWorldPoint,minDist);
        if (closestObjectNdx != -1) {
            Matrix inverseTransform = sceneObjects[closestObjectNdx].invTransform;
            Point fromPointObjectSpace= inverseTransform*isectWorldPoint;
            Vector rayObjectSpace = inverseTransform*reflectedRay;
            Vector normal = sceneObjects[closestObjectNdx].shape->findIsectNormal(fromPointObjectSpace, rayObjectSpace, minDist);
            normal = transpose(inverseTransform) * normal;
            normal.normalize();
            isectWorldPoint = isectWorldPoint + minDist*reflectedRay;
            reflectedColor=calculateColor(sceneObjects[closestObjectNdx], normal, reflectedRay, isectWorldPoint,--recurseDepth);
            reflectedColor[0]*=Or[0];
            reflectedColor[1]*=Or[1];
            reflectedColor[2]*=Or[2];
        }
    }
	return color+reflectedColor;
	//return reflectedColor;
}

int getClosestObjectNdx(Vector ray,Point start,double& minDist){
    int closestObject = -1;
    for (unsigned int k = 0; k < sceneObjects.size(); k++) {
        Shape* shape = sceneObjects[k].shape;
        double curDist = shape->Intersect(start, ray, sceneObjects[k].transform);
        if ((curDist < minDist) && (curDist > 0) && !(IN_RANGE(curDist, 0))) {
            minDist = curDist;
            closestObject = k;
        }
    }
    return closestObject;
}
void renderPixel(int i,int j){

    Vector ray = generateRay(i, j);
    double minDist = MIN_ISECT_DISTANCE;
    int closestObject=getClosestObjectNdx(ray,camera->GetEyePoint(),minDist);
    if (isectOnly == 1) {
        setpixel(pixels, i, j, 255, 255, 255);
    }
    else {
        Point previous_color=Point(1,1,1);
        Point color;
        if (closestObject != -1) {
            Matrix inverseTransform = sceneObjects[closestObject].invTransform;
            Point eyePointObjectSpace = inverseTransform*camera->GetEyePoint();
            Vector rayObjectSpace = inverseTransform*ray;
            Vector normal = sceneObjects[closestObject].shape->findIsectNormal(eyePointObjectSpace, rayObjectSpace, minDist);
            normal = transpose(inverseTransform) * normal;
            normal.normalize();
            Point isectWorldPoint = camera->GetEyePoint() + minDist*ray;
            color=calculateColor(sceneObjects[closestObject], normal, ray, isectWorldPoint,maxDepth);
        }
        color = color * 255;
        setpixel(pixels, i, j, color[0], color[1], color[2]);
    }
}

void callback_start(int id) {
	cout << "start button clicked!" << endl;

	if (parser == NULL) {
		cout << "no scene loaded yet" << endl;
		return;
	}

	pixelWidth = screenWidth;
	pixelHeight = screenHeight;

	cout << "Processing " << pixelWidth << " columns and " << pixelHeight << " rows." << endl;

	updateCamera();

	if (pixels != NULL) {
		delete pixels;
	}
	pixels = new GLubyte[pixelWidth  * pixelHeight * 3];
	memset(pixels, 0, pixelWidth  * pixelHeight * 3);

    SceneGlobalData gData;
    parser->getGlobalData(gData);
    double now=clock();
    string space(1024,' ');
	for (int i = 0; i < pixelWidth; i++) {
		for (int j = 0; j < pixelHeight; j++) {
			// cout << "computing: " << i << ", " << j << endl;
            renderPixel(i,j);
        }

		double percent = (((double)i) / ((double)pixelWidth))*100.0;
        double timeTaken=(clock()-now)/CLOCKS_PER_SEC;
        double eta=(double(timeTaken)/double(i+1))*(double(pixelWidth-i-1));
        char progress[1024];
		sprintf(progress,"% 3f%% [rendering line % 3d] run time %f, eta %fs",
                percent, i,timeTaken,eta);
        cout<<progress<<'\r';
		fflush(stdout);
	}
	printf("% 3f%% [rendered]\
                                                        \n", 100.0);
	glutPostRedisplay();
}



void callback_load(int id) {
	if (filenameTextField == NULL) {
		return;
	}
	printf ("%s\n", filenameTextField->get_text());

	if (parser != NULL) {
		delete parser;
	}
	parser = new SceneParser (filenamePath);
    bool success=parser->parse();
	cout << "success? " << success << endl;

    if(success){
	    setupCamera();
	    sceneObjects.clear();
	    Matrix identity;
	    flattenScene(parser->getRootNode(), identity);
    }
}

Shape* findShape(int shapeType) {
	Shape* shape = NULL;
	switch (shapeType) {
	case SHAPE_CUBE:
		shape = cube;
		break;
	case SHAPE_CYLINDER:
		shape = cylinder;
		break;
	case SHAPE_CONE:
		shape = cone;
		break;
	case SHAPE_SPHERE:
		shape = sphere;
		break;
	case SHAPE_SPECIAL1:
		shape = cube;
		break;
	default:
		shape = cube;
	}
	return shape;
}


/***************************************** myGlutIdle() ***********/

bool initialLoad;//TODO delete
void myGlutIdle(void)
{
	/* According to the GLUT specification, the current window is
	undefined during an idle callback.  So we need to explicitly change
	it if necessary */
	if (glutGetWindow() != main_window)
		glutSetWindow(main_window);
    if(!initialLoad){//TODO delete
        callback_load(0);//delete
        callback_start(0);//delete
        initialLoad=true;//delete
    }//delete

	glutPostRedisplay();
}


/**************************************** myGlutReshape() *************/

void myGlutReshape(int x, int y)
{
	float xy_aspect;

	xy_aspect = (float)x / (float)y;
	glViewport(0, 0, x, y);
	camera->SetScreenSize(x, y);

	screenWidth = x;
	screenHeight = y;

	glutPostRedisplay();
}


/***************************************** setupCamera() *****************/
void setupCamera()
{
	SceneCameraData cameraData;
	parser->getCameraData(cameraData);

	camera->Reset();
	camera->SetViewAngle(cameraData.heightAngle);
	if (cameraData.isDir == true) {
		camera->Orient(cameraData.pos, cameraData.look, cameraData.up);
	}
	else {
		camera->Orient(cameraData.pos, cameraData.lookAt, cameraData.up);
	}

	viewAngle = camera->GetViewAngle();
	Point eyeP = camera->GetEyePoint();
	Vector lookV = camera->GetLookVector();
	eyeX = eyeP[0];
	eyeY = eyeP[1];
	eyeZ = eyeP[2];
	lookX = lookV[0];
	lookY = lookV[1];
	lookZ = lookV[2];
	camRotU = 0;
	camRotV = 0;
	camRotW = 0;
	GLUI_Master.sync_live_all();
}

void updateCamera()
{
	camera->Reset();

	Point guiEye (eyeX, eyeY, eyeZ);
	Point guiLook(lookX, lookY, lookZ);
	camera->SetViewAngle(viewAngle);
    Vector upVector=camera->GetUpVector();
	camera->Orient(guiEye, guiLook, upVector);
	camera->RotateU(camRotU);
	camera->RotateV(camRotV);
	camera->RotateW(camRotW);

	camera->computeCamera2WorldMatrix();
}

void flattenScene(SceneNode* node, Matrix compositeMatrix)
{
	std::vector<SceneTransformation*> transVec = node->transformations;
	for (unsigned int i = 0; i<transVec.size(); i++) {
		SceneTransformation* trans = transVec[i];
		switch (trans->type) {
		case TRANSFORMATION_TRANSLATE:
			compositeMatrix = compositeMatrix * trans_mat(trans->translate);
			break;
		case TRANSFORMATION_SCALE:
			compositeMatrix = compositeMatrix * scale_mat(trans->scale);
			break;
		case TRANSFORMATION_ROTATE:
			compositeMatrix = compositeMatrix * rot_mat(trans->rotate, trans->angle);
			break;
		case TRANSFORMATION_MATRIX:
			compositeMatrix = compositeMatrix * trans->matrix;
			break;
		}
	}

	SceneGlobalData globalData;
	parser->getGlobalData(globalData);
	std::vector<ScenePrimitive*> objectVec = node->primitives;
	for (unsigned int j = 0; j<objectVec.size(); j++) {
		SceneObject tempObj;
		tempObj.transform = compositeMatrix;
		tempObj.invTransform = invert(compositeMatrix);
        //K's already applied

		tempObj.material = objectVec[j]->material;
		tempObj.material.cAmbient.r     *= globalData.ka;
		tempObj.material.cAmbient.g     *= globalData.ka;
		tempObj.material.cAmbient.b     *= globalData.ka;
		tempObj.material.cDiffuse.r     *= globalData.kd;
		tempObj.material.cDiffuse.g     *= globalData.kd;
		tempObj.material.cDiffuse.b     *= globalData.kd;
		tempObj.material.cSpecular.r    *= globalData.ks;
		tempObj.material.cSpecular.g    *= globalData.ks;
		tempObj.material.cSpecular.b    *= globalData.ks;
		tempObj.material.cReflective.r  *= globalData.ks;
		tempObj.material.cReflective.g  *= globalData.ks;
		tempObj.material.cReflective.b  *= globalData.ks;
		tempObj.material.cTransparent.r *= globalData.kt;
		tempObj.material.cTransparent.g *= globalData.kt;
		tempObj.material.cTransparent.b *= globalData.kt;

		tempObj.shape = findShape(objectVec[j]->type);
        tempObj.mapTexture();//mapps the texture for that object if it exists
		sceneObjects.push_back(tempObj);
	}

	std::vector<SceneNode*> childrenVec = node->children;
	for (unsigned int k = 0; k<childrenVec.size(); k++) {
		flattenScene(childrenVec[k], compositeMatrix);
	}
}

/***************************************** myGlutDisplay() *****************/

void myGlutDisplay(void)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (parser == NULL) {
		return;
	}

	if (pixels == NULL) {
		return;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glDrawPixels(pixelWidth, pixelHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	glutSwapBuffers();
}

void onExit()
{
	delete cube;
	delete cylinder;
	delete cone;
	delete sphere;
	delete camera;
	if (parser != NULL) {
		delete parser;
	}
	if (pixels != NULL) {
		delete pixels;
	}
}

/**************************************** main() ********************/

int main(int argc, char* argv[])
{
	atexit(onExit);

	/****************************************/
	/*   Initialize GLUT and create window  */
	/****************************************/

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowPosition(50, 50);
	glutInitWindowSize(500, 500);

	main_window = glutCreateWindow("CSI 4341: Assignment 5");
	glutDisplayFunc(myGlutDisplay);
	glutReshapeFunc(myGlutReshape);

	glShadeModel (GL_SMOOTH);

	glEnable(GL_DEPTH_TEST);

	// Specular reflections will be off without this, since OpenGL calculates
	// specular highlights using an infinitely far away camera by default, not
	// the actual location of the camera
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

	// Show all ambient light for the entire scene (not one by default)
	GLfloat one[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, one);

	glPolygonOffset(1, 1);



	/****************************************/
	/*         Here's the GLUI code         */
	/****************************************/

	GLUI* glui = GLUI_Master.create_glui("GLUI");

	filenameTextField = new GLUI_EditText( glui, "Filename:", filenamePath);
	filenameTextField->set_w(300);
	glui->add_button("Load", 0, callback_load);
	glui->add_button("Start!", 0, callback_start);
	glui->add_checkbox("Isect       Only" , &isectOnly);
	glui->add_checkbox("Ambient     Light", &ambientOn);
	glui->add_checkbox("Diffuse     Light", &diffuseOn);
	glui->add_checkbox("Specular    Light", &specularOn);
	glui->add_checkbox("Transparent Light", &transparentOn);
	
	
	GLUI_Panel *recursionPanel = glui->add_panel("Recursion");
	(new GLUI_Spinner(recursionPanel, "Recursion Depth:", &maxDepth))
		->set_int_limits(0, 6);
	
	
	GLUI_Panel *camera_panel = glui->add_panel("Camera");
	(new GLUI_Spinner(camera_panel, "RotateV:", &camRotV))
		->set_int_limits(-179, 179);
	(new GLUI_Spinner(camera_panel, "RotateU:", &camRotU))
		->set_int_limits(-179, 179);
	(new GLUI_Spinner(camera_panel, "RotateW:", &camRotW))
		->set_int_limits(-179, 179);
	(new GLUI_Spinner(camera_panel, "Angle:", &viewAngle))
		->set_int_limits(1, 179);

	glui->add_column_to_panel(camera_panel, true);

	GLUI_Spinner* eyex_widget = glui->add_spinner_to_panel(camera_panel, "EyeX:", GLUI_SPINNER_FLOAT, &eyeX);
	eyex_widget->set_float_limits(-10, 10);
	GLUI_Spinner* eyey_widget = glui->add_spinner_to_panel(camera_panel, "EyeY:", GLUI_SPINNER_FLOAT, &eyeY);
	eyey_widget->set_float_limits(-10, 10);
	GLUI_Spinner* eyez_widget = glui->add_spinner_to_panel(camera_panel, "EyeZ:", GLUI_SPINNER_FLOAT, &eyeZ);
	eyez_widget->set_float_limits(-10, 10);

	GLUI_Spinner* lookx_widget = glui->add_spinner_to_panel(camera_panel, "LookX:", GLUI_SPINNER_FLOAT, &lookX);
	lookx_widget->set_float_limits(-10, 10);
	GLUI_Spinner* looky_widget = glui->add_spinner_to_panel(camera_panel, "LookY:", GLUI_SPINNER_FLOAT, &lookY);
	looky_widget->set_float_limits(-10, 10);
	GLUI_Spinner* lookz_widget = glui->add_spinner_to_panel(camera_panel, "LookZ:", GLUI_SPINNER_FLOAT, &lookZ);
	lookz_widget->set_float_limits(-10, 10);

	glui->add_button("Quit", 0, (GLUI_Update_CB)exit);

	glui->set_main_gfx_window(main_window);

	/* We register the idle callback with GLUI, *not* with GLUT */
	GLUI_Master.set_glutIdleFunc(myGlutIdle);

	glutMainLoop();

	return EXIT_SUCCESS;
}



