#include <GL/glut.h>
#include <stdlib.h>
#include "colors.h"
#include "gltools_extracted.h"
#include "VectorMath.cpp"
#include <array>
#include <vector>



// sta³e do obs³ugi menu podrêcznego
enum
{
	//2. (Ostros³up SUB Czworoœcian) OR (Kula AND Torus)

	CSG_A, // tylko obiekt Kula
	CSG_B, // tylko obiekt Torus
	CSG_C, // tylko obiekt Ostros³up
	CSG_D,//Tylko czworoœcian
	CSG_A_OR_B,
	CSG_A_OR_C,
	CSG_A_AND_B,
	CSG_A_SUB_B,
	CSG_B_SUB_A,
	CSG_C_SUB_A,
	CSG_OSTROSLUP_SUB_Czworoscian_OR_KULA_AND_TORUS, //zadanie
	FULL_WINDOW, // aspekt obrazu - ca³e okno
	ASPECT_1_1, // aspekt obrazu 1:1
	EXIT // wyjœcie
};

// aspekt obrazu

int aspect = FULL_WINDOW;

// rozmiary bry³y obcinania

const GLdouble left = -2.0;
const GLdouble right = 2.0;
const GLdouble bottom = -2.0;
const GLdouble top = 2.0;
const GLdouble nearr = 3.0;
const GLdouble farr = 7.0;

// k¹ty obrotu sceny

GLfloat rotatex = 0.0;
GLfloat rotatey = 0.0;

// wskaŸnik naciœniêcia lewego przycisku myszki

int button_state = GLUT_UP;

// po³o¿enie kursora myszki

int button_x, button_y;

// identyfikatory list wyœwietlania

GLint A, B, C, D;

// rodzaj operacji CSG

int csg_op = CSG_A_OR_B;

// ustawienie bufora szablonowego tak, aby wydzieliæ i wyœwietliæ
// te elementy obiektu A, które znajduj¹ siê we wnêtrzu obiektu B;
// stronê (przedni¹ lub tyln¹) wyszukiwanych elementów obiektu A
// okreœla parametr cull_face
void CrateObject(int numberOfSides, double radius, float heigth, float x, float y, const  GLfloat*color)
{
	//góra
	glTranslatef(x, y, 0);
	glBegin(GL_POLYGON);
	//wysokoœæ sto¿ka
	glVertex3f(.0f, heigth, .0f);
	glColor3fv(color);
	for (int i = 0; i < numberOfSides; i++)
	{
		float xx = x + radius * sin(2.0*3.14*i / numberOfSides);
		float yy = y + radius * cos(2.0*3.14*i / numberOfSides);
		glVertex3f(xx, .0f, yy);
		glColor3fv(color);
	}
	//namalowanie pierwszego wieszko³ka do do zamkniêcia figury
	glVertex3f(x + radius * sin(0), .0f, y + radius * cos(0));
	glColor3fv(color);
	glEnd();
	//malowanie podstawy
	glRotatef(180, 0, 0, 1);
	glBegin(GL_POLYGON);
	for (int i = 0; i < numberOfSides; i++)
	{
		float xx = x + radius * sin(2.0*3.14*i / numberOfSides);
		float yy = y + radius * cos(2.0*3.14*i / numberOfSides);
		glVertex3f(xx, .0f, yy);
		glColor4fv(color);
	}
	glEnd();
}
void Inside(GLint A, GLint B, GLenum cull_face, GLenum stencil_func)
{
	// pocz¹tkowo rysujemy obiekt A w buforze g³êbokoœci przy
	// wy³¹czonym zapisie sk³adowych RGBA do bufora kolorów

	// w³¹czenie testu bufora g³êbokoœci
	glEnable(GL_DEPTH_TEST);

	// wy³¹czenie zapisu sk³adowych RGBA do bufora kolorów
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// rysowanie wybranej strony wielok¹tów
	glCullFace(cull_face);

	// wyœwietlenie obiektu A
	glCallList(A);

	// nastêpnie przy u¿yciu bufora szablonowego wykrywamy te elementy
	// obiektu A, które znajduj¹ siê wewn¹trz obiektu B; w tym celu
	// zawartoœæ bufora szablonowego jest zwiêkszana o 1, wszêdzie gdzie
	// bêd¹ przednie strony wielok¹tów sk³adaj¹cych siê na obiekt B

	// wy³¹czenie zapisu do bufora g³êbokoœci
	glDepthMask(GL_FALSE);

	// w³¹czenie bufora szablonowego
	glEnable(GL_STENCIL_TEST);

	// test bufora szablonowego
	glStencilFunc(GL_ALWAYS, 0, 0);

	// okreœlenie operacji na buforze szablonowym
	glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

	// rysowanie tylko przedniej strony wielok¹tów
	glCullFace(GL_BACK);

	// wyœwietlenie obiektu B
	glCallList(B);

	// w kolejnym etapie zmniejszamy zawartoœæ bufora szablonowego o 1
	// wszêdzie tam, gdzie s¹ tylne strony wielok¹tów obiektu B

	// okreœlenie operacji na buforze szablonowym
	glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);

	// rysowanie tylko tylnej strony wielok¹tów
	glCullFace(GL_FRONT);

	// wyœwietlenie obiektu B
	glCallList(B);

	// dalej wyœwietlamy te elementy obiektu A, które
	// znajduj¹ siê we wnêtrzu obiektu B

	// w³¹czenie zapisu do bufora g³êbokoœci
	glDepthMask(GL_TRUE);

	// w³¹czenie zapisu sk³adowych RGBA do bufora kolorów
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	// test bufora szablonowego
	glStencilFunc(stencil_func, 0, 1);

	// wy³¹czenie testu bufora g³êbokoœci
	glDisable(GL_DEPTH_TEST);

	// rysowanie wybranej strony wielok¹tów
	glCullFace(cull_face);

	// wyœwietlenie obiektu A
	glCallList(A);

	// wy³¹czenie bufora szablonowego
	glDisable(GL_STENCIL_TEST);
}

// funkcja generuj¹ca scenê 3D

void DisplayScene()
{
	// kolor t³a - zawartoœæ bufora koloru
	glClearColor(1.0, 1.0, 1.0, 1.0);

	// czyszczenie bufora koloru, bufora g³êbokoœci i bufora szablonowego
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// wybór macierzy modelowania
	glMatrixMode(GL_MODELVIEW);

	// macierz modelowania = macierz jednostkowa
	glLoadIdentity();

	// przesuniêcie uk³adu wspó³rzêdnych obiektów do œrodka bry³y odcinania
	glTranslatef(0, 0, -(nearr + farr) / 2);

	// obroty ca³ej sceny
	glRotatef(rotatex, 1.0, 0.0, 0.0);
	glRotatef(rotatey, 0.0, 1.0, 0.0);

	// w³¹czenie oœwietlenia
	glEnable(GL_LIGHTING);

	// w³¹czenie œwiat³a GL_LIGHT0
	glEnable(GL_LIGHT0);

	// w³¹czenie automatycznej normalizacji wektorów normalnych
	glEnable(GL_NORMALIZE);

	// w³¹czenie obs³ugi w³aœciwoœci materia³ów
	glEnable(GL_COLOR_MATERIAL);

	// w³aœciwoœci materia³u okreœlone przez kolor wierzcho³ków
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	// w³¹czenie rysowania wybranej strony wielok¹tów
	glEnable(GL_CULL_FACE);

	// operacja CSG - tylko obiekt A
	if (csg_op == CSG_A)
	{
		// w³¹czenie testu bufora g³êbokoœci
		glEnable(GL_DEPTH_TEST);

		// wyœwietlenie obiektu A
		glCallList(A);

		// wy³¹czenie testu bufora g³êbokoœci
		glDisable(GL_DEPTH_TEST);
	}

	// operacja CSG - tylko obiekt B
	if (csg_op == CSG_B)
	{
		// w³¹czenie testu bufora g³êbokoœci
		glEnable(GL_DEPTH_TEST);

		// wyœwietlenie obiektu B
		glCallList(B);

		// wy³¹czenie testu bufora g³êbokoœci
		glDisable(GL_DEPTH_TEST);
	}
	if (csg_op == CSG_C)
	{

		// w³¹czenie testu bufora g³êbokoœci
		glEnable(GL_DEPTH_TEST);
		// wyœwietlenie obiektu C
		glCallList(C);

		// wy³¹czenie testu bufora g³êbokoœci
		glDisable(GL_DEPTH_TEST);
	}
	if (csg_op == CSG_D)/////////////////////////////////////////////////////////////////--------------------------------------------------------------------
	{

		// w³¹czenie testu bufora g³êbokoœci
		glEnable(GL_DEPTH_TEST);
		// wyœwietlenie obiektu D
		glCallList(D);

		// wy³¹czenie testu bufora g³êbokoœci
		glDisable(GL_DEPTH_TEST);
	}

	// operacja CSG A lub B
	if (csg_op == CSG_A_OR_B)
	{
		// w³¹czenie testu bufora g³êbokoœci
		glEnable(GL_DEPTH_TEST);

		// wyœwietlenie obiektu A i B
		glCallList(A);
		glCallList(B);

		// wy³¹czenie testu bufora g³êbokoœci
		glDisable(GL_DEPTH_TEST);
	}
	if (csg_op == CSG_A_OR_C)
	{
		// w³¹czenie testu bufora g³êbokoœci
		glEnable(GL_DEPTH_TEST);
		// wyœwietlenie obiektu A i B
		glCallList(A);
		glCallList(C);

		// wy³¹czenie testu bufora g³êbokoœci
		glDisable(GL_DEPTH_TEST);
	}
	// operacja CSG A AND B
	if (csg_op == CSG_A_AND_B)
	{
		// elementy obiektu A znajduj¹ce siê we wnêtrzu B
		Inside(A, B, GL_BACK, GL_NOTEQUAL);
		// wy³¹czenie zapisu sk³adowych RGBA do bufora kolorów
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		// w³¹czenie testu bufora g³êbokoœci
		glEnable(GL_DEPTH_TEST);
		// wy³¹czenie bufora szablonowego
		glDisable(GL_STENCIL_TEST);
		// wybór funkcji do testu bufora g³êbokoœci
		glDepthFunc(GL_ALWAYS);
		// wyœwietlenie obiektu B
		glCallList(B);
		// wybór funkcji do testu bufora g³êbokoœci
		glDepthFunc(GL_LESS);
		// elementy obiektu B znajduj¹ce siê we wnêtrzu A
		Inside(B, A, GL_BACK, GL_NOTEQUAL);
	}
	// operacja CSG A SUB B
	if (csg_op == CSG_A_SUB_B)
	{
		// elementy obiektu A znajduj¹ce siê we wnêtrzu B
		Inside(A, B, GL_FRONT, GL_NOTEQUAL);
		// wy³¹czenie zapisu sk³adowych RGBA do bufora kolorów
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		// w³¹czenie testu bufora g³êbokoœci
		glEnable(GL_DEPTH_TEST);
		// wy³¹czenie bufora szablonowego
		glDisable(GL_STENCIL_TEST);
		// wybór funkcji do testu bufora g³êbokoœci
		glDepthFunc(GL_ALWAYS);
		// wyœwietlenie obiektu B
		glCallList(B);
		// wybór funkcji do testu bufora g³êbokoœci
		glDepthFunc(GL_LESS);
		// elementy obiektu B znajduj¹ce siê we wnêtrzu A
		Inside(B, A, GL_BACK, GL_EQUAL);
	}
	// operacja CSG B SUB A
	if (csg_op == CSG_B_SUB_A)
	{
		// elementy obiektu B znajduj¹ce siê we wnêtrzu A
		Inside(B, A, GL_FRONT, GL_NOTEQUAL);
		// wy³¹czenie zapisu sk³adowych RGBA do bufora kolorów
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		// w³¹czenie testu bufora g³êbokoœci
		glEnable(GL_DEPTH_TEST);
		// wy³¹czenie bufora szablonowego
		glDisable(GL_STENCIL_TEST);
		// wybór funkcji do testu bufora g³êbokoœci
		glDepthFunc(GL_ALWAYS);
		// wyœwietlenie obiektu A
		glCallList(A);
		// wybór funkcji do testu bufora g³êbokoœci
		glDepthFunc(GL_LESS);
		// elementy obiektu A znajduj¹ce siê we wnêtrzu B
		Inside(A, B, GL_BACK, GL_EQUAL);
	}
	if (csg_op == CSG_C_SUB_A)
	{
		// elementy obiektu B znajduj¹ce siê we wnêtrzu A
		Inside(C, A, GL_FRONT, GL_NOTEQUAL);
		// wy³¹czenie zapisu sk³adowych RGBA do bufora kolorów
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		// w³¹czenie testu bufora g³êbokoœci
		glEnable(GL_DEPTH_TEST);
		// wy³¹czenie bufora szablonowego
		glDisable(GL_STENCIL_TEST);
		// wybór funkcji do testu bufora g³êbokoœci
		glDepthFunc(GL_ALWAYS);
		// wyœwietlenie obiektu A
		glCallList(A);
		// wybór funkcji do testu bufora g³êbokoœci
		glDepthFunc(GL_LESS);
		// elementy obiektu A znajduj¹ce siê we wnêtrzu B
		Inside(A, C, GL_BACK, GL_EQUAL);
	}
	if (csg_op == CSG_OSTROSLUP_SUB_Czworoscian_OR_KULA_AND_TORUS)// A kula B Torus C ostros³up D czworoscian
	{

		Inside(C, D, GL_FRONT, GL_NOTEQUAL);

		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDepthFunc(GL_ALWAYS);
		glCallList(D);
		glDepthFunc(GL_LESS);
		Inside(D, C, GL_BACK, GL_EQUAL);
		/*	Inside(D, C, GL_FRONT, GL_NOTEQUAL);

		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDepthFunc(GL_ALWAYS);
		glCallList(C);
		glDepthFunc(GL_LESS);
		Inside(C, D, GL_BACK, GL_EQUAL);*/

		Inside(B, A, GL_BACK, GL_NOTEQUAL);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDepthFunc(GL_ALWAYS);
		glCallList(A);
		glDepthFunc(GL_LESS);
		Inside(A, B, GL_BACK, GL_NOTEQUAL);
		glDisable(GL_DEPTH_TEST); 
	}
	// skierowanie poleceñ do wykonania
	glFlush();

	// zamiana buforów koloru
	glutSwapBuffers();
}
// zmiana wielkoœci okna
void Reshape(int width, int height)
{
	// obszar renderingu - ca³e okno
	glViewport(0, 0, width, height);

	// wybór macierzy rzutowania
	glMatrixMode(GL_PROJECTION);

	// macierz rzutowania = macierz jednostkowa
	glLoadIdentity();

	// parametry bry³y obcinania
	if (aspect == ASPECT_1_1)
	{
		// wysokoœæ okna wiêksza od wysokoœci okna
		if (width < height && width > 0)
			glFrustum(left, right, bottom * height / width, top * height / width, nearr, farr);
		else

			// szerokoœæ okna wiêksza lub równa wysokoœci okna
			if (width >= height && height > 0)
				glFrustum(left * width / height, right * width / height, bottom, top, nearr, farr);
	}
	else
		glFrustum(left, right, bottom, top, nearr, farr);

	// generowanie sceny 3D
	DisplayScene();
}
// obs³uga przycisków myszki
void MouseButton(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON)
	{
		// zapamiêtanie stanu lewego przycisku myszki
		button_state = state;

		// zapamiêtanie po³o¿enia kursora myszki
		if (state == GLUT_DOWN)
		{
			button_x = x;
			button_y = y;
		}
	}
}
// obs³uga ruchu kursora myszki
void MouseMotion(int x, int y)
{
	if (button_state == GLUT_DOWN)
	{
		rotatey += 30 * (right - left) / glutGet(GLUT_WINDOW_WIDTH) *(x - button_x);
		button_x = x;
		rotatex -= 30 * (top - bottom) / glutGet(GLUT_WINDOW_HEIGHT) *(button_y - y);
		button_y = y;
		glutPostRedisplay();
	}
}

// obs³uga menu podrêcznego

void Menu(int value)
{
	switch (value)
	{
		// operacja CSG
	case CSG_A:
	case CSG_B:
	case CSG_C:
	case CSG_D:
	case CSG_A_OR_B:
	case CSG_A_OR_C:
	case CSG_A_AND_B:
	case CSG_A_SUB_B:
	case CSG_B_SUB_A:
	case CSG_C_SUB_A:
	case CSG_OSTROSLUP_SUB_Czworoscian_OR_KULA_AND_TORUS:
		csg_op = value;
		DisplayScene();
		break;

		// obszar renderingu - ca³e okno
	case FULL_WINDOW:
		aspect = FULL_WINDOW;
		Reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
		break;

		// obszar renderingu - aspekt 1:1
	case ASPECT_1_1:
		aspect = ASPECT_1_1;
		Reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
		break;

		// wyjœcie
	case EXIT:
		exit(0);
	}
}

// utworzenie list wyœwietlania

void GenerateDisplayLists()
{
	// generowanie identyfikatora pierwszej listy wyœwietlania
	A = glGenLists(1);

	// pierwsza lista wyœwietlania
	glNewList(A, GL_COMPILE);

	// czerwony szeœcian
	glColor4fv(Red);
	glutSolidSphere(1, 100, 100);

	// koniec pierwszej listy wyœwietlania
	glEndList();

	// generowanie identyfikatora drugiej listy wyœwietlania
	B = glGenLists(1);

	// druga lista wyœwietlania
	glNewList(B, GL_COMPILE);

	// zielona kula
	glColor4fv(Green);
	glutSolidTorus(.2, 1, 100, 100);

	glEndList();
	// generowanie identyfikatora trzeciej listy wyœwietlania
	C = glGenLists(1);
	glNewList(C, GL_COMPILE);
	glColor4fv(Yellow);
	CrateObject(5, 2, 2, 0, 0, Yellow);
	glEndList();
	// koniec drugiej listy wyœwietlania

	// generowanie identyfikatora drugiej listy wyœwietlania-------------------------------------------------
	D = glGenLists(1);
	glNewList(D, GL_COMPILE);
	glColor4fv(Blue);
	glutSolidTetrahedron();

	glEndList();
}

int main(int argc, char * argv[])
{
	// inicjalizacja biblioteki GLUT
	glutInit(&argc, argv);

	// inicjalizacja bufora ramki
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);

	// rozmiary g³ównego okna programu
	glutInitWindowSize(500, 500);

	// utworzenie g³ównego okna programu
	glutCreateWindow("CSG");

	// do³¹czenie funkcji generuj¹cej scenê 3D
	glutDisplayFunc(DisplayScene);

	// do³¹czenie funkcji wywo³ywanej przy zmianie rozmiaru okna
	glutReshapeFunc(Reshape);

	// obs³uga przycisków myszki
	glutMouseFunc(MouseButton);

	// obs³uga ruchu kursora myszki
	glutMotionFunc(MouseMotion);

	// utworzenie podmenu - Operacja CSG
	int MenuCSGOp = glutCreateMenu(Menu);
	glutAddMenuEntry("A", CSG_A);
	glutAddMenuEntry("B", CSG_B);
	glutAddMenuEntry("C", CSG_C);
	glutAddMenuEntry("D", CSG_D);
	glutAddMenuEntry("A OR B", CSG_A_OR_B);
	glutAddMenuEntry("A OR c", CSG_A_OR_C);
	glutAddMenuEntry("CSG_C_SUB_A", CSG_C_SUB_A);
	glutAddMenuEntry("A AND B", CSG_A_AND_B);
	glutAddMenuEntry("A SUB B", CSG_A_SUB_B);
	glutAddMenuEntry("B SUB A", CSG_B_SUB_A);
	glutAddMenuEntry("Zadanie", CSG_OSTROSLUP_SUB_Czworoscian_OR_KULA_AND_TORUS);

	// utworzenie podmenu - Aspekt obrazu
	int MenuAspect = glutCreateMenu(Menu);
#ifdef WIN32

	glutAddMenuEntry("Aspekt obrazu - ca³e okno", FULL_WINDOW);
#else

	glutAddMenuEntry("Aspekt obrazu - cale okno", FULL_WINDOW);
#endif

	glutAddMenuEntry("Aspekt obrazu 1:1", ASPECT_1_1);

	// menu g³ówne
	glutCreateMenu(Menu);
	glutAddSubMenu("Operacja CSG", MenuCSGOp);

#ifdef WIN32

	glutAddSubMenu("Aspekt obrazu", MenuAspect);
	glutAddMenuEntry("Wyjœcie", EXIT);
#else

	glutAddSubMenu("Aspekt obrazu", MenuAspect);
	glutAddMenuEntry("Wyjscie", EXIT);
#endif

	// okreœlenie przycisku myszki obs³uguj¹cego menu podrêczne
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// utworzenie list wyœwietlania
	GenerateDisplayLists();

	// wprowadzenie programu do obs³ugi pêtli komunikatów
	glutMainLoop();
	return 0;
}