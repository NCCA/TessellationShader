#include <QMouseEvent>
#include <QGuiApplication>

#include "NGLScene.h"
#include <ngl/NGLInit.h>
#include <ngl/VAOFactory.h>
#include <ngl/SimpleIndexVAO.h>
#include <ngl/ShaderLib.h>
#include <QFont>

//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for x/y translation with mouse movement
//----------------------------------------------------------------------------------------------------------------------
const static float INCREMENT=0.01f;
//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for the wheel zoom
//----------------------------------------------------------------------------------------------------------------------
const static float ZOOM=0.1f;

NGLScene::NGLScene()
{
  // re-size the widget to that of the parent (in this case the GLFrame passed in on construction)
  m_rotate=false;
  // mouse rotation values set to 0
  m_spinXFace=0;
  m_spinYFace=0;
  setTitle("Qt5 Simple NGL Demo");
}


NGLScene::~NGLScene()
{
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
}




void NGLScene::resizeGL(int _w , int _h)
{
  m_project=ngl::perspective(45.0f,(float)width()/height(),0.05f,350.0f);
  m_width=_w*devicePixelRatio();
  m_height=_h*devicePixelRatio();
}

void NGLScene::initializeGL()
{
  ngl::NGLInit::instance();
  std::cerr << "OpenGL Version : " << glGetString(GL_VERSION) << std::endl;

  m_text.reset( new ngl::Text(QFont("Arial",16)));
  m_text->setColour(1,1,1);
  m_text->setScreenSize(width(),height());

  glClearColor(0.4f, 0.4f, 0.4f, 1.0f);			   // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // we need to initialise the NGL lib, under windows and linux we also need to
  // initialise GLEW, under windows this needs to be done in the app as well
  // as the lib hence the WIN32 define
    // now to load the shader and set the values
  // grab an instance of shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();

  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from(0,2,2);
  ngl::Vec3 to(0,0,0);
  ngl::Vec3 up(0,1,0);
  // now load to our new camera
  m_view=ngl::lookAt(from,to,up);
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
 m_project=ngl::perspective(50,720.0f/576.0f,0.05f,350);

  shader->createShaderProgram("Tess");
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader("TessVertex",ngl::ShaderType::VERTEX);
  shader->attachShader("TessFragment",ngl::ShaderType::FRAGMENT);
  shader->attachShader("TessGeom",ngl::ShaderType::GEOMETRY);
  shader->attachShader("TessControl",ngl::ShaderType::TESSCONTROL);
  shader->attachShader("TessEval",ngl::ShaderType::TESSEVAL);
  // attach the source
  shader->loadShaderSource("TessVertex","shaders/tessvert.glsl");
  shader->loadShaderSource("TessFragment","shaders/tessfrag.glsl");
  shader->loadShaderSource("TessGeom","shaders/tessgeom.glsl");
  shader->loadShaderSource("TessControl","shaders/tesscontrol.glsl");
  shader->loadShaderSource("TessEval","shaders/tesseval.glsl");
  // add them to the program
  shader->attachShaderToProgram("Tess","TessVertex");
  shader->attachShaderToProgram("Tess","TessFragment");
  shader->attachShaderToProgram("Tess","TessGeom");
  shader->attachShaderToProgram("Tess","TessEval");
  shader->attachShaderToProgram("Tess","TessControl");

  // compile the shaders
  shader->compileShader("TessVertex");
  shader->compileShader("TessFragment");
  shader->compileShader("TessGeom");
  shader->compileShader("TessEval");
  shader->compileShader("TessControl");

  // now we have associated this data we can link the shader
  shader->linkProgramObject("Tess");
  shader->use("Tess");
  shader->autoRegisterUniforms("Tess");
  shader->printRegisteredUniforms("Tess");
  shader->setUniform("AmbientMaterial",0.1f, 0.1f, 0.1f);
  shader->setUniform("DiffuseMaterial",0.8f, 0.0f, 0.0f);
  shader->setUniform("LightPosition",1.0f,1.0f, 1.0f);
  //glPatchParameteri(GL_PATCH_VERTICES, 16);
  createIcosahedron();
  m_innerLevel=1.0;
  m_outerLevel=1.0;
}


void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();

  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  ngl::Mat4 M;
  M=m_mouseGlobalTX*m_transform.getMatrix();
  MV=  m_view*M;
  MVP= m_project*MV;
  normalMatrix=MV;
  normalMatrix.inverse().transpose();
  shader->setUniform("MVP",MVP);
  shader->setUniform("TessLevelInner",m_innerLevel);
  shader->setUniform("TessLevelOuter",m_outerLevel);
  shader->setUniform("NormalMatrix",normalMatrix);
}

void NGLScene::paintGL()
{
  // clear the screen and depth buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0,0,m_width,m_height);
  // grab an instance of the shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["Tess"]->use();

  ngl::Mat4 rotX;
  ngl::Mat4 rotY;
  // create the rotation matrices
  rotX.rotateX(m_spinXFace);
  rotY.rotateY(m_spinYFace);
  // multiply the rotations
  m_mouseGlobalTX=rotY*rotX;
  // add the translations
  m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
  m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
  m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;
  // set this in the TX stack
  loadMatricesToShader();
  m_vao->bind();
  m_vao->draw();
  m_vao->unbind();

  m_text->setColour(1.0f,1.0f,1.0f);
  QString text=QString("1 2 change inner tesselation level  current value %1").arg(m_innerLevel);
  m_text->renderText(10,18,text);
  text=QString("3 4 change outer tesselation level  current value %1").arg(m_outerLevel);
  m_text->renderText(10,34,text);
}


void NGLScene::createIcosahedron()
{
     const static GLubyte  Faces[] = {
        2, 1, 0,
        3, 2, 0,
        4, 3, 0,
        5, 4, 0,
        1, 5, 0,

        11, 6,  7,
        11, 7,  8,
        11, 8,  9,
        11, 9,  10,
        11, 10, 6,

        1, 2, 6,
        2, 3, 7,
        3, 4, 8,
        4, 5, 9,
        5, 1, 10,

        2,  7, 6,
        3,  8, 7,
        4,  9, 8,
        5, 10, 9,
        1, 6, 10 };

     float Verts[] = {
         0.000f,  0.000f,  1.000f,
         0.894f,  0.000f,  0.447f,
         0.276f,  0.851f,  0.447f,
        -0.724f,  0.526f,  0.447f,
        -0.724f, -0.526f,  0.447f,
         0.276f, -0.851f,  0.447f,
         0.724f,  0.526f, -0.447f,
        -0.276f,  0.851f, -0.447f,
        -0.894f,  0.000f, -0.447f,
        -0.276f, -0.851f, -0.447f,
         0.724f, -0.526f, -0.447f,
         0.000f,  0.000f, -1.000f };

    auto IndexCount = sizeof(Faces) / sizeof(Faces[0]);
    m_vao=ngl::VAOFactory::createVAO(ngl::simpleIndexVAO, GL_PATCHES);
    m_vao->bind();
    m_vao->setData(ngl::SimpleIndexVAO::VertexData(  sizeof(Verts),Verts[0],sizeof(Faces),&Faces[0],GL_UNSIGNED_BYTE,GL_STATIC_DRAW));
    m_vao->setVertexAttributePointer(0,3,GL_FLOAT,0,0);
    m_vao->setNumIndices(IndexCount);

    m_vao->unbind();
}


//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseMoveEvent (QMouseEvent * _event)
{
  // note the method buttons() is the button state when event was called
  // this is different from button() which is used to check which button was
  // pressed when the mousePress/Release event is generated
  if(m_rotate && _event->buttons() == Qt::LeftButton)
  {
    int diffx=_event->x()-m_origX;
    int diffy=_event->y()-m_origY;
    m_spinXFace += (float) 0.5f * diffy;
    m_spinYFace += (float) 0.5f * diffx;
    m_origX = _event->x();
    m_origY = _event->y();
    update();

  }
        // right mouse translate code
  else if(m_translate && _event->buttons() == Qt::RightButton)
  {
    int diffX = (int)(_event->x() - m_origXPos);
    int diffY = (int)(_event->y() - m_origYPos);
    m_origXPos=_event->x();
    m_origYPos=_event->y();
    m_modelPos.m_x += INCREMENT * diffX;
    m_modelPos.m_y -= INCREMENT * diffY;
    update();

   }
}


//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mousePressEvent ( QMouseEvent * _event)
{
  // this method is called when the mouse button is pressed in this case we
  // store the value where the maouse was clicked (x,y) and set the Rotate flag to true
  if(_event->button() == Qt::LeftButton)
  {
    m_origX = _event->x();
    m_origY = _event->y();
    m_rotate =true;
  }
  // right mouse translate mode
  else if(_event->button() == Qt::RightButton)
  {
    m_origXPos = _event->x();
    m_origYPos = _event->y();
    m_translate=true;
  }

}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseReleaseEvent ( QMouseEvent * _event )
{
  // this event is called when the mouse button is released
  // we then set Rotate to false
  if (_event->button() == Qt::LeftButton)
  {
    m_rotate=false;
  }
        // right mouse translate mode
  if (_event->button() == Qt::RightButton)
  {
    m_translate=false;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::wheelEvent(QWheelEvent *_event)
{

	// check the diff of the wheel position (0 means no change)
	if(_event->delta() > 0)
	{
		m_modelPos.m_z+=ZOOM;
	}
	else if(_event->delta() <0 )
	{
		m_modelPos.m_z-=ZOOM;
	}
	update();
}
//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quite
  case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
  // turn on wirframe rendering
  case Qt::Key_W : glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); break;
  // turn off wire frame
  case Qt::Key_S : glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); break;
  // show full screen
  case Qt::Key_F : showFullScreen(); break;
  // show windowed
  case Qt::Key_N : showNormal(); break;
  case Qt::Key_1 : updateInnerTess(-1); break;
  case Qt::Key_2 : updateInnerTess(1); break;

  case Qt::Key_3 : updateOuterTess(-1); break;
  case Qt::Key_4 : updateOuterTess(1); break;
  case Qt::Key_Space : reset(); break;

  default : break;
  }
  // finally update the GLWindow and re-draw
  //if (isExposed())
    update();
}

void NGLScene::updateInnerTess(float _v)
{
    m_innerLevel+=_v;
    m_innerLevel=std::min(64.0f, std::max(1.0f, m_innerLevel));
}

void NGLScene::updateOuterTess(float _v)
{
    m_outerLevel+=_v;
    m_outerLevel=std::min(64.0f, std::max(1.0f, m_outerLevel));
}

