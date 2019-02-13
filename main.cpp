#include "tgaimage.h"
#include "utilitaire.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <utility>
#include <algorithm>

using namespace std;

struct Point {
  float x;
  float y;
  float z;
};

struct Triangle {
  Point a;
  Point b;
  Point c;
  //coordonéne des textures
  Point texA;
  Point texB;
  Point texC;

} ;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const int largeur = 800;
const int hauteur = 800;
const int profondeur = 800;
const int tailleMatrix = 4; // taille des matrice : projection
std::vector<Point> positions;
std::vector<Point> posTex;
std::vector<Triangle> triangles;
float zBuffer[largeur][hauteur];

TGAImage texture;
Point camera;
Point center;

//matrice identité 4x4
void matriceIdentite(float res[][tailleMatrix]) {

	for (int i = 0; i < tailleMatrix;i++) {
		res[i][i] = 1;
	}

}

void pointToMatrice(float x, float y, float z,float res[][tailleMatrix]) {
	res[0][0] = x;
	res[0][1] = y;
	res[0][2] = z;
	res[0][tailleMatrix-1] = 1;
}

void matriceToPoint(float res[][tailleMatrix], Point &p) {
	p.x = res[0][0] / res[0][tailleMatrix - 1];// *(largeur / 2) + (largeur / 2);
	p.y = res[0][1] / res[0][tailleMatrix - 1];// *(hauteur / 2) + (hauteur / 2);
	p.z = res[0][2] / res[0][tailleMatrix - 1];// *(profondeur / 2) + (profondeur / 2);
}

void matriceMult(float a[][tailleMatrix] ,float b[][tailleMatrix] ,float res[][tailleMatrix]) {
	float tmp[1][4];

	for (int k = 0;k < tailleMatrix;k++) {
		tmp[0][k] = b[0][k];
		res[0][k] = 0;
	}

	for (int i = 0; i < tailleMatrix;i++) {
		for (int j = 0;j < tailleMatrix;j++) {
			res[0][i] += a[i][j] * tmp[0][j];
		}
	}
}

void ajout4D(float a[][tailleMatrix],float valeur, int pos) {
	a[tailleMatrix-1][pos] = valeur;
}

void normalize(Point &p) {
	//normalisation
	float norme = std::sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
	p.x = p.x / norme;
	p.y = p.y / norme;
	p.z = p.z / norme;
}

void soustraction(Point &a, Point &b, Point &res) {
	res.x = a.x - b.x;
	res.y = a.y - b.y;
	res.z = a.z - b.z;
}

void cross(Point &a, Point &b, Point &res) {
	res.x = a.y * b.z - a.z * b.y;
	res.y = b.z * a.x - b.x * a.z;
	res.z = a.x * b.y - a.y * b.x;
}

void lookAt(float res[][4]) {
	Point up;
	up.x = 0;
	up.y = 1;
	up.z = 0;

	Point z;
	soustraction(camera, center, z);
	normalize(z);

	Point x;
	cross(up,z,x);
	normalize(x);

	Point y;
	cross(z, x, y);
	normalize(y);

	matriceIdentite(res);

	//resultat
	res[0][0] = x.x;
	res[0][1] = x.y;
	res[0][2] = x.z;
	res[0][3] = -center.x;

	res[1][0] = y.x;
	res[1][1] = y.y;
	res[1][2] = y.z;
	res[1][3] = -center.y;

	res[2][0] = z.x;
	res[2][1] = z.y;
	res[2][2] = z.z;
	res[2][3] = -center.z;

}

void viewport(int x, int y, int w, int h, float res[][4]) {
	
	matriceIdentite(res);
	res[0][3] = x + w / 2.f;
	res[1][3] = y + h / 2.f;
	res[2][3] = profondeur / 2.f;

	res[0][0] = w / 2.f;
	res[1][1] = h / 2.f;
	res[2][2] = profondeur / 2.f;
}

//explosion d'une chaine de caractère
std::vector<std::string> explode(std::string const & s, char delimitation)
{
    std::vector<std::string> res; // resultat
    std::istringstream iss(s);

    for (std::string token; std::getline(iss, token, delimitation); )
    {
            res.push_back(std::move(token));
    }

    return res;
}

//lecture du fichier et recupératio ndes informations
void lectureFichier(std::string name, TGAImage &image){

    ifstream file;

	std::vector<std::string> exp;

	//model view
	float modelView[4][4] = {};
	lookAt(modelView);

    file.open(name.c_str());
    if (!file){
        cout << "Pas de fichier\n";
        exit(1);
    }

    //declarations
    std::string tmp;
    std::vector<std::string> ex;
    float a,b,c;
    int t1,t2,t3,tex1,tex2,tex3;
    Point p;
    Triangle t;
	
	//projection
	float matrix[4][4] = {};
	matriceIdentite(matrix);
	Point n;
	soustraction(camera,center,n);
	float norme = std::sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
	ajout4D(matrix, -1 / norme, 2);

	//viewport
	float viewPort[tailleMatrix][tailleMatrix] = {};
	viewport(largeur / 8, hauteur / 8, largeur * 3 / 4, hauteur * 3 / 4, viewPort);

    //pour chaque ligne du fichier
    while (!file.eof()) {
        std::getline(file,tmp);

        //recuperation des points
        if(tmp[0] == 'v' && (tmp[1] == ' ' || tmp[1] == 't')){
            ex = explode(tmp,' ');
            std::istringstream stream;

            int i = 1;//decallage de 2 pour "vt"
            if(tmp[1] == 't') i = 2;

            stream.str(ex[i]);
            stream >> a;
            stream.str(ex[i+1]);
            stream.seekg(0);
            stream >> b;
            stream.str(ex[i+2]);
			stream.seekg(0);
            stream >> c;

            p = Point();


            if(tmp[1] != 't'){
				//projection
				float point[1][4] = {}; //on stock ici le point courant que l'on traite
				pointToMatrice(a , b , c , point);
				//matriceMult(modelView, point, point);
				matriceMult(matrix, point, point);
				matriceMult(viewPort,point,point);
				matriceToPoint(point, p);
				//cout << p.x << "  " << p.y << "  " << p.z << "\n";
            }
            else{
				p.x = a*texture.get_width();
				p.y = b*texture.get_height();
				p.z = c;
            }

            if(tmp[1] == 't')
				posTex.push_back(p);
            else positions.push_back(p);

        }
        //recuperations des triangles a tracer
        if(tmp[0] == 'f' && tmp[1] == ' '){
            ex = explode(tmp,' ');
			exp = explode(ex[1], '/');

			//point a
            std::istringstream stream(exp[0]);
            stream >> t1;
			stream.str(exp[1]);
			stream.seekg(0);
			stream >> tex1;

			//point b
			exp = explode(ex[2], '/');
			stream.str(exp[0]);
			stream.seekg(0);
            stream >> t2;
			stream.str(exp[1]);
			stream.seekg(0);
			stream >> tex2;

			//point c
			exp = explode(ex[3], '/');
            stream.str(exp[0]);
			stream.seekg(0);
            stream >> t3;
			stream.str(exp[1]);
			stream.seekg(0);
			stream >> tex3;

			//creation du triangle
            t = Triangle();
            t.a = positions[t1-1];
            t.b = positions[t2-1];
            t.c = positions[t3-1];

			t.texA = posTex[tex1 - 1];
			t.texB = posTex[tex2 - 1];
			t.texC = posTex[tex3 - 1];

            triangles.push_back(t);
        }
    }
    file.close();
}

//colorisation d'un triangle
void colorTriangle(TGAImage &image, int numTriangle,float intensity){

    Triangle t = triangles[numTriangle];
    Point p1,p2,p3,p,tex;
    TGAColor color;

    p.z = t.a.z;

    float div,alpha,beta,gamma;

    int minX = std::min(std::min(t.a.x,t.b.x),t.c.x);
    int maxX = std::max(std::max(t.a.x,t.b.x),t.c.x);

    int minY = std::min(std::min(t.a.y,t.b.y),t.c.y);
    int maxY = std::max(std::max(t.a.y,t.b.y),t.c.y);
    for(int i = minX;i < maxX;i++){
        p.x = i;
        for(int j = minY; j < maxY;j++){
            p.y = j;

            p1 = t.a;
            p2 = t.b;
            p3 = t.c;

            float div = (float)(((int)p2.y - (int)p3.y) * ((int)p1.x - (int)p3.x) + ((int)p3.x - (int)p2.x) * ((int)p1.y - (int)p3.y));

            if(div != 0){
                alpha = (((int)p2.y - (int)p3.y) * ((int)p.x - (int)p3.x) + ((int)p3.x - (int)p2.x) * ((int)p.y - (int)p3.y)) / div;
                beta = (((int)p3.y - (int)p1.y) * ((int)p.x - (int)p3.x) + ((int)p1.x - (int)p3.x) * ((int)p.y - (int)p3.y)) / div;
            }
            else{
                beta = -1;
                alpha = -1;
            }

            gamma = 1.0f - alpha - beta;
            if(gamma >= -.001 && alpha >= -.001 && beta >= -.001){
                if(p.z > zBuffer[(int)p.x][(int)p.y]){
                    zBuffer[(int)p.x][(int)p.y] = p.z;
                    tex.x = alpha * t.texA.x + beta * t.texB.x + gamma * t.texC.x;
                    tex.y = alpha * t.texA.y + beta * t.texB.y + gamma * t.texC.y;

                    color = texture.get(tex.x,tex.y);
                    image.set(p.x,p.y,color.operator *(intensity));
                }
            }
        }
    }
}

//colorisation de tout les triangle avec backface culling
void remplissageTriangle(TGAImage &image){
    Triangle t;
    TGAColor tga;
    float tmp;
    Point light_dir;
	light_dir.x = 0;
	light_dir.y = 0;
	light_dir.z = 1;
    for(int i =0;i < triangles.size();i++){
        t = triangles[i];

        Point vector1;
        Point vector2;
        Point normale;

		soustraction(t.b, t.a, vector1);
		soustraction(t.c, t.a, vector2);

        //creation de la normale
		cross(vector1 ,vector2 ,normale);

        //normalisation
		normalize(normale);

        tmp = 0;
		tmp += light_dir.x * normale.x;
		tmp += light_dir.y * normale.y;
		tmp += light_dir.z * normale.z;

        if(tmp > 0)
            colorTriangle(image,i,tmp);
    }
}

int main(int argc, char *argv[])
{
	//remplissage zbuffer
	for (int i = 0; i < largeur;i++) {
		for (int j = 0; j < hauteur;j++) {
            zBuffer[i][j] = std::numeric_limits<float>::min();
		}
	}

	//camera
	camera.x = 1;
	camera.y = 1;
	camera.z = 3;

	center.x = 0;
	center.y = 0;
	center.z = 0;

	//creation image
    TGAImage image(largeur, hauteur, TGAImage::RGB);

	//lecture texture
	texture.read_tga_file("../african_head_diffuse.tga");
    texture.flip_vertically();

	//lecture obj
    lectureFichier("../african_head.obj",image);

	//remplissage de l'image
	remplissageTriangle(image);
    image.flip_vertically();

	//ecriture
    image.write_tga_file("output.tga");

    return 0;
}