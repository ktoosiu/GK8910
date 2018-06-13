#include <GL/glut.h>
#include <stdlib.h>
#include "colors.h"
#include "gltools_extracted.h"
#include "VectorMath.cpp"
#include <array>
#include <vector>



// sta�e do obs�ugi menu podr�cznego
enum
{
	//2. (Ostros�up SUB Czworo�cian) OR (Kula AND Torus)

	CSG_A, // tylko obiekt Kula
	CSG_B, // tylko obiekt Torus
	CSG_C, // tylko obiekt Ostros�up
	CSG_D,//Tylko czworo�cian
	CSG_A_OR_B,
	CSG_A_OR_C,
	CSG_A_AND_B,
	CSG_A_SUB_B,
	CSG_B_SUB_A,
	CSG_C_SUB_A,
	CSG_OSTROSLUP_SUB_Czworoscian_OR_KULA_AND_TORUS, //zadanie
	FULL_WINDOW, // aspekt obrazu - ca�e okno
	ASPECT_1_1, // aspekt obrazu 1:1
	EXIT // wyj�cie
};

// aspekt obrazu

int aspect = FULL_WINDOW;

// rozmiary bry�y obcinania

const GLdouble left = -2.0;
const GLdouble right = 2.0;
const GLdouble bottom = -2.0;
const GLdouble top = 2.0;
const GLdouble nearr = 3.0;
const GLdouble farr = 7.0;

// k�ty obrotu sceny

GLfloat rotatex = 0.0;
GLfloat rotatey = 0.0;

// wska�nik naci�ni�cia lewego przycisku myszki

int button_state = GLUT_UP;

// po�o�enie kursora myszki

int button_x, button_y;

// identyfikatory list wy�wietlania

GLint A, B, C, D;

// rodzaj operacji CSG

int csg_op = CSG_A_OR_B;

// ustawienie bufora szablonowego tak, aby wydzieli� i wy�wietli�
// te elementy obiektu A, kt�re znajduj� si� we wn�trzu obiektu B;
// stron� (przedni� lub tyln�) wyszukiwanych element�w obiektu A
// okre�la parametr cull_face
void CrateObject(int numberOfSides, double radius, float heigth, float x, float y, const  GLfloat*color)
{
	//g�ra
	glTranslatef(x, y, 0);
	glBegin(GL_POLYGON);
	//wysoko�� sto�ka
	glVertex3f(.0f, heigth, .0f);
	glColor3fv(color);
	for (int i = 0; i < numberOfSides; i++)
	{
		float xx = x + radius * sin(2.0*3.14*i / numberOfSides);
		float yy = y + radius * cos(2.0*3.14*i / numberOfSides);
		glVertex3f(xx, .0f, yy);
		glColor3fv(color);
	}
	//namalowanie pierwszego wieszko�ka do do zamkni�cia figury
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
	// pocz�tkowo rysujemy obiekt A w buforze g��boko�ci przy
	// wy��czonym zapisie sk�adowych RGBA do bufora kolor�w

	// w��czenie testu bufora g��boko�ci
	glEnable(GL_DEPTH_TEST);

	// wy��czenie zapisu sk�adowych RGBA do bufora kolor�w
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// rysowanie wybranej strony wielok�t�w
	glCullFace(cull_face);

	// wy�wietlenie obiektu A
	glCallList(A);

	// nast�pnie przy u�yciu bufora szablonowego wykrywamy te elementy
	// obiektu A, kt�re znajduj� si� wewn�trz obiektu B; w tym celu
	// zawarto�� bufora szablonowego jest zwi�kszana o 1, wsz�dzie gdzie
	// b�d� przednie strony wielok�t�w sk�adaj�cych si� na obiekt B

	// wy��czenie zapisu do bufora g��boko�ci
	glDepthMask(GL_FALSE);

	// w��czenie bufora szablonowego
	glEnable(GL_STENCIL_TEST);

	// test bufora szablonowego
	glStencilFunc(GL_ALWAYS, 0, 0);

	// okre�lenie operacji na buforze szablonowym
	glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

	// rysowanie tylko przedniej strony wielok�t�w
	glCullFace(GL_BACK);

	// wy�wietlenie obiektu B
	glCallList(B);

	// w kolejnym etapie zmniejszamy zawarto�� bufora szablonowego o 1
	// wsz�dzie tam, gdzie s� tylne strony wielok�t�w obiektu B

	// okre�lenie operacji na buforze szablonowym
	glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);

	// rysowanie tylko tylnej strony wielok�t�w
	glCullFace(GL_FRONT);

	// wy�wietlenie obiektu B
	glCallList(B);

	// dalej wy�wietlamy te elementy obiektu A, kt�re
	// znajduj� si� we wn�trzu obiektu B

	// w��czenie zapisu do bufora g��boko�ci
	glDepthMask(GL_TRUE);

	// w��czenie zapisu sk�adowych RGBA do bufora kolor�w
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	// test bufora szablonowego
	glStencilFunc(stencil_func, 0, 1);

	// wy��czenie testu bufora g��boko�ci
	glDisable(GL_DEPTH_TEST);

	// rysowanie wybranej strony wielok�t�w
	glCullFace(cull_face);

	// wy�wietlenie obiektu A
	glCallList(A);

	// wy��czenie bufora szablonowego
	glDisable(GL_STENCIL_TEST);
}

// funkcja generuj�ca scen� 3D

void DisplayScene()
{
	// kolor t�a - zawarto�� bufora koloru
	glClearColor(1.0, 1.0, 1.0, 1.0);

	// czyszczenie bufora koloru, bufora g��boko�ci i bufora szablonowego
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// wyb�r macierzy modelowania
	glMatrixMode(GL_MODELVIEW);

	// macierz modelowania = macierz jednostkowa
	glLoadIdentity();

	// przesuni�cie uk�adu wsp�rz�dnych obiekt�w do �rodka bry�y odcinania
	glTranslatef(0, 0, -(nearr + farr) / 2);

	// obroty ca�ej sceny
	glRotatef(rotatex, 1.0, 0.0, 0.0);
	glRotatef(rotatey, 0.0, 1.0, 0.0);

	// w��czenie o�wietlenia
	glEnable(GL_LIGHTING);

	// w��czenie �wiat�a GL_LIGHT0
	glEnable(GL_LIGHT0);

	// w��czenie automatycznej normalizacji wektor�w normalnych
	glEnable(GL_NORMALIZE);

	// w��czenie obs�ugi w�a�ciwo�ci materia��w
	glEnable(GL_COLOR_MATERIAL);

	// w�a�ciwo�ci materia�u okre�lone przez kolor wierzcho�k�w
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	// w��czenie rysowania wybranej strony wielok�t�w
	glEnable(GL_CULL_FACE);

	// operacja CSG - tylko obiekt A
	if (csg_op == CSG_A)
	{
		// w��czenie testu bufora g��boko�ci
		glEnable(GL_DEPTH_TEST);

		// wy�wietlenie obiektu A
		glCallList(A);

		// wy��czenie testu bufora g��boko�ci
		glDisable(GL_DEPTH_TEST);
	}

	// operacja CSG - tylko obiekt B
	if (csg_op == CSG_B)
	{
		// w��czenie testu bufora g��boko�ci
		glEnable(GL_DEPTH_TEST);

		// wy�wietlenie obiektu B
		glCallList(B);

		// wy��czenie testu bufora g��boko�ci
		glDisable(GL_DEPTH_TEST);
	}
	if (csg_op == CSG_C)
	{

		// w��czenie testu bufora g��boko�ci
		glEnable(GL_DEPTH_TEST);
		// wy�wietlenie obiektu C
		glCallList(C);

		// wy��czenie testu bufora g��boko�ci
		glDisable(GL_DEPTH_TEST);
	}
	if (csg_op == CSG_D)/////////////////////////////////////////////////////////////////--------------------------------------------------------------------
	{

		// w��czenie testu bufora g��boko�ci
		glEnable(GL_DEPTH_TEST);
		// wy�wietlenie obiektu D
		glCallList(D);

		// wy��czenie testu bufora g��boko�ci
		glDisable(GL_DEPTH_TEST);
	}

	// operacja CSG A lub B
	if (csg_op == CSG_A_OR_B)
	{
		// w��czenie testu bufora g��boko�ci
		glEnable(GL_DEPTH_TEST);

		// wy�wietlenie obiektu A i B
		glCallList(A);
		glCallList(B);

		// wy��czenie testu bufora g��boko�ci
		glDisable(GL_DEPTH_TEST);
	}
	if (csg_op == CSG_A_OR_C)
	{
		// w��czenie testu bufora g��boko�ci
		glEnable(GL_DEPTH_TEST);
		// wy�wietlenie obiektu A i B
		glCallList(A);
		glCallList(C);

		// wy��czenie testu bufora g��boko�ci
		glDisable(GL_DEPTH_TEST);
	}
	// operacja CSG A AND B
	if (csg_op == CSG_A_AND_B)
	{
		// elementy obiektu A znajduj�ce si� we wn�trzu B
		Inside(A, B, GL_BACK, GL_NOTEQUAL);
		// wy��czenie zapisu sk�adowych RGBA do bufora kolor�w
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		// w��czenie testu bufora g��boko�ci
		glEnable(GL_DEPTH_TEST);
		// wy��czenie bufora szablonowego
		glDisable(GL_STENCIL_TEST);
		// wyb�r funkcji do testu bufora g��boko�ci
		glDepthFunc(GL_ALWAYS);
		// wy�wietlenie obiektu B
		glCallList(B);
		// wyb�r funkcji do testu bufora g��boko�ci
		glDepthFunc(GL_LESS);
		// elementy obiektu B znajduj�ce si� we wn�trzu A
		Inside(B, A, GL_BACK, GL_NOTEQUAL);
	}
	// operacja CSG A SUB B
	if (csg_op == CSG_A_SUB_B)
	{
		// elementy obiektu A znajduj�ce si� we wn�trzu B
		Inside(A, B, GL_FRONT, GL_NOTEQUAL);
		// wy��czenie zapisu sk�adowych RGBA do bufora kolor�w
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		// w��czenie testu bufora g��boko�ci
		glEnable(GL_DEPTH_TEST);
		// wy��czenie bufora szablonowego
		glDisable(GL_STENCIL_TEST);
		// wyb�r funkcji do testu bufora g��boko�ci
		glDepthFunc(GL_ALWAYS);
		// wy�wietlenie obiektu B
		glCallList(B);
		// wyb�r funkcji do testu bufora g��boko�ci
		glDepthFunc(GL_LESS);
		// elementy obiektu B znajduj�ce si� we wn�trzu A
		Inside(B, A, GL_BACK, GL_EQUAL);
	}
	// operacja CSG B SUB A
	if (csg_op == CSG_B_SUB_A)
	{
		// elementy obiektu B znajduj�ce si� we wn�trzu A
		Inside(B, A, GL_FRONT, GL_NOTEQUAL);
		// wy��czenie zapisu sk�adowych RGBA do bufora kolor�w
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		// w��czenie testu bufora g��boko�ci
		glEnable(GL_DEPTH_TEST);
		// wy��czenie bufora szablonowego
		glDisable(GL_STENCIL_TEST);
		// wyb�r funkcji do testu bufora g��boko�ci
		glDepthFunc(GL_ALWAYS);
		// wy�wietlenie obiektu A
		glCallList(A);
		// wyb�r funkcji do testu bufora g��boko�ci
		glDepthFunc(GL_LESS);
		// elementy obiektu A znajduj�ce si� we wn�trzu B
		Inside(A, B, GL_BACK, GL_EQUAL);
	}
	if (csg_op == CSG_C_SUB_A)
	{
		// elementy obiektu B znajduj�ce si� we wn�trzu A
		Inside(C, A, GL_FRONT, GL_NOTEQUAL);
		// wy��czenie zapisu sk�adowych RGBA do bufora kolor�w
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		// w��czenie testu bufora g��boko�ci
		glEnable(GL_DEPTH_TEST);
		// wy��czenie bufora szablonowego
		glDisable(GL_STENCIL_TEST);
		// wyb�r funkcji do testu bufora g��boko�ci
		glDepthFunc(GL_ALWAYS);
		// wy�wietlenie obiektu A
		glCallList(A);
		// wyb�r funkcji do testu bufora g��boko�ci
		glDepthFunc(GL_LESS);
		// elementy obiektu A znajduj�ce si� we wn�trzu B
		Inside(A, C, GL_BACK, GL_EQUAL);
	}
	if (csg_op == CSG_OSTROSLUP_SUB_Czworoscian_OR_KULA_AND_TORUS)// A kula B Torus C ostros�up D czworoscian
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
	// skierowanie polece� do wykonania
	glFlush();

	// zamiana bufor�w koloru
	glutSwapBuffers();
}
// zmiana wielko�ci okna
void Reshape(int width, int height)
{
	// obszar renderingu - ca�e okno
	glViewport(0, 0, width, height);

	// wyb�r macierzy rzutowania
	glMatrixMode(GL_PROJECTION);

	// macierz rzutowania = macierz jednostkowa
	glLoadIdentity();

	// parametry bry�y obcinania
	if (aspect == ASPECT_1_1)
	{
		// wysoko�� okna wi�ksza od wysoko�ci okna
		if (width < height && width > 0)
			glFrustum(left, right, bottom * height / width, top * height / width, nearr, farr);
		else

			// szeroko�� okna wi�ksza lub r�wna wysoko�ci okna
			if (width >= height && height > 0)
				glFrustum(left * width / height, right * width / height, bottom, top, nearr, farr);
	}
	else
		glFrustum(left, right, bottom, top, nearr, farr);

	// generowanie sceny 3D
	DisplayScene();
}
// obs�uga przycisk�w myszki
void MouseButton(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON)
	{
		// zapami�tanie stanu lewego przycisku myszki
		button_state = state;

		// zapami�tanie po�o�enia kursora myszki
		if (state == GLUT_DOWN)
		{
			button_x = x;
			button_y = y;
		}
	}
}
// obs�uga ruchu kursora myszki
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

// obs�uga menu podr�cznego

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

		// obszar renderingu - ca�e okno
	case FULL_WINDOW:
		aspect = FULL_WINDOW;
		Reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
		break;

		// obszar renderingu - aspekt 1:1
	case ASPECT_1_1:
		aspect = ASPECT_1_1;
		Reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
		break;

		// wyj�cie
	case EXIT:
		exit(0);
	}
}

// utworzenie list wy�wietlania

void GenerateDisplayLists()
{
	// generowanie identyfikatora pierwszej listy wy�wietlania
	A = glGenLists(1);

	// pierwsza lista wy�wietlania
	glNewList(A, GL_COMPILE);

	// czerwony sze�cian
	glColor4fv(Red);
	glutSolidSphere(1, 100, 100);

	// koniec pierwszej listy wy�wietlania
	glEndList();

	// generowanie identyfikatora drugiej listy wy�wietlania
	B = glGenLists(1);

	// druga lista wy�wietlania
	glNewList(B, GL_COMPILE);

	// zielona kula
	glColor4fv(Green);
	glutSolidTorus(.2, 1, 100, 100);

	glEndList();
	// generowanie identyfikatora trzeciej listy wy�wietlania
	C = glGenLists(1);
	glNewList(C, GL_COMPILE);
	glColor4fv(Yellow);
	CrateObject(5, 2, 2, 0, 0, Yellow);
	glEndList();
	// koniec drugiej listy wy�wietlania

	// generowanie identyfikatora drugiej listy wy�wietlania-------------------------------------------------
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

	// rozmiary g��wnego okna programu
	glutInitWindowSize(500, 500);

	// utworzenie g��wnego okna programu
	glutCreateWindow("CSG");

	// do��czenie funkcji generuj�cej scen� 3D
	glutDisplayFunc(DisplayScene);

	// do��czenie funkcji wywo�ywanej przy zmianie rozmiaru okna
	glutReshapeFunc(Reshape);

	// obs�uga przycisk�w myszki
	glutMouseFunc(MouseButton);

	// obs�uga ruchu kursora myszki
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

	glutAddMenuEntry("Aspekt obrazu - ca�e okno", FULL_WINDOW);
#else

	glutAddMenuEntry("Aspekt obrazu - cale okno", FULL_WINDOW);
#endif

	glutAddMenuEntry("Aspekt obrazu 1:1", ASPECT_1_1);

	// menu g��wne
	glutCreateMenu(Menu);
	glutAddSubMenu("Operacja CSG", MenuCSGOp);

#ifdef WIN32

	glutAddSubMenu("Aspekt obrazu", MenuAspect);
	glutAddMenuEntry("Wyj�cie", EXIT);
#else

	glutAddSubMenu("Aspekt obrazu", MenuAspect);
	glutAddMenuEntry("Wyjscie", EXIT);
#endif

	// okre�lenie przycisku myszki obs�uguj�cego menu podr�czne
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// utworzenie list wy�wietlania
	GenerateDisplayLists();

	// wprowadzenie programu do obs�ugi p�tli komunikat�w
	glutMainLoop();
	return 0;
}