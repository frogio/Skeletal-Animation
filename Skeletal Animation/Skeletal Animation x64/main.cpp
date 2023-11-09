#include "SkeletalModel.h"
#include "glut.h"


void glReshape(int w, int h);
void Display();

float _zoom = 15.0f;
unsigned char _btnState[3] = { 0, };
int last_x = 0;
int last_y = 0;

float _rotate_x = 0.0f;
float _rotate_y = 0.001f;
float curTime = 0.f;

void Timer();
void Mouse(int button, int state, int x, int y);
void mouseMotion(int x, int y);
void keyboard(unsigned char ch, int x, int y);

SkeletalModel model("obj/serena_rig_poses.fbx");


void main(int argc, char *argv[]) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowPosition(50, 100);		
	glutInitWindowSize(800, 600);			
	glutCreateWindow("Skeletal Animation");	
	glutReshapeFunc(glReshape);
	glutDisplayFunc(Display);				
	glutMouseFunc(Mouse);					
	glutMotionFunc(mouseMotion);
	glutIdleFunc(Timer);
	glutKeyboardFunc(keyboard);

	glEnable(GL_DEPTH_TEST);				
	glutMainLoop();							

}


void glReshape(int w, int h) {
	if (w == 0)
		h = 1;

	glViewport(0, 0, w, h);	// �׷��� ����ġ���� ������
	glMatrixMode(GL_PROJECTION);
	// ���� ����� �Ǵ� ��� ���� (ī�޶� ��ǥ ���)
	glLoadIdentity();
	// �ʱ�ȭ
	gluPerspective(90.0f, (float)w / h, 0.1f, 100.0f);
	// Perspective ��� ����, 90���� fov, ��Ⱦ��, near, far�� (ī�޶� ��ġ)

	glMatrixMode(GL_MODELVIEW);
	// ���� ����� �Ǵ� ��� ���� (���� ��ǥ ����� ��ȯ��.)	

	// OpenGL�� �⺻ ����
}

void Display() {

	glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	// ��������� �ҷ���, (��� ������ �ʱⰪ)
	glTranslatef(0.0f, 0.0f, -_zoom);
	
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glRotatef(_rotate_x, 1, 0, 0);
	glRotatef(_rotate_y, 0, 0, 1);
	glScalef(0.4f, 0.4f, 0.4f);
	
	GLfloat pos0[4] = { 0.0f, 10.0f, 1000.0f, 10.0f };			// ���� ��ġ
	GLfloat ambient0[4] = { 0.8f, 0.8f, 0.8f, 0.8f };
	GLfloat diffuse0[4] = { 0.5f, 0.5f, 0.5f, 0.5f };			// �л걤�� ����
	GLfloat specular0[4] = { 1.0f, 1.0f, 1.0f, 1.0f };			// ��¦�� ����

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient0);
	glLightfv(GL_LIGHT0, GL_POSITION, pos0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular0);

	glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180.0f);
	glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 0.0f);				// �� ��������
	


	model.drawAnimation(curTime);
	//model.drawSolid();
	//model.drawSolidProc(skipDrawing);
	//model.drawAnimationPoint(curTime);

	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);

	glutSwapBuffers();
	// ���� ���۸�
}

void mouseMotion(int x, int y) {

	int diff_x = x - last_x;
	int diff_y = y - last_y;
	// ���콺�� �巡�� �� �� �Ÿ��� ���

	if (_btnState[2]) {
		_zoom -= (float)0.05f * diff_x;
	}
	else if (_btnState[0]) {
		_rotate_x += (float)0.05f * diff_y;
		_rotate_y += (float)0.05f * diff_x;
	}

	glutPostRedisplay();
}

void Timer() {

	curTime += 0.01f;
	glutPostRedisplay();


}


void Mouse(int button, int state, int x, int y) {
	last_x = x;
	last_y = y;
	// ���콺�� Ŭ���� �� ������ ��ǥ

	switch (button) {
	case GLUT_LEFT_BUTTON:
		printf("left button\n");
		_btnState[0] = ((GLUT_DOWN == state) ? 1 : 0);
		break;
	case GLUT_MIDDLE_BUTTON:
		printf("middle button\n");
		_btnState[1] = ((GLUT_DOWN == state) ? 1 : 0);
		break;
	case GLUT_RIGHT_BUTTON:
		printf("right button\n");
		_btnState[2] = ((GLUT_DOWN == state) ? 1 : 0);
		//one = false;
		break;
	}
	glutPostRedisplay();
	// ����� _zoom ���� _translate�� ��ǥ������ Redisplay ����
}

void keyboard(unsigned char ch, int x, int y) {

}