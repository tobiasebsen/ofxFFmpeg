#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
	ofGLWindowSettings settings;
	//settings.setGLVersion(4, 1); /// < select your GL Version here
	ofCreateWindow(settings);
	//ofSetupOpenGL(1024,768,OF_WINDOW);			// <-------- setup the GL context

	cout << "====== OpenGL driver ======" << endl;
	cout << "Vendor:   " << glGetString(GL_VENDOR) << endl;
	cout << "Renderer: " << glGetString(GL_RENDERER) << endl;
	cout << "Version:  " << glGetString(GL_VERSION) << endl;
	cout << "GLSL:     " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	cout << "===========================" << endl << endl;

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp());

}
