#include "tgaimage.h"
#include <QCoreApplication>
#include "utilitaire.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <utility>

using namespace std;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const int largeur = 1000;
const int hauteur = 1000;
std::vector<std::vector<int>> positions;
std::vector<std::vector<int>> triangles;

void drawLine(int x0, int y0,int x1, int y1, TGAImage &image, TGAColor color){

    bool steep = false;
       if (std::abs(x0-x1)<std::abs(y0-y1)) {
           std::swap(x0, y0);
           std::swap(x1, y1);
           steep = true;
       }
       if (x0>x1) {
           std::swap(x0, x1);
           std::swap(y0, y1);
       }
       int dx = x1-x0;
       int dy = y1-y0;
       int derror2 = std::abs(dy)*2;
       int error2 = 0;
       int y = y0;
       for (int x=x0; x<=x1; x++) {
           if (steep) {
               image.set(y, x, color);
           } else {
               image.set(x, y, color);
           }
           error2 += derror2;
           if (error2 > dx) {
               y += (y1>y0?1:-1);
               error2 -= dx*2;
           }
       }
}

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

void lectureFichier(std::string name, TGAImage &image){

    ifstream file;

    file.open(name.c_str());
    if (!file){
        cout << "Pas de fichier\n";
        exit(1);
    }

    //declarations
    std::string tmp;
    std::vector<std::string> ex;
    float a,b;
    int t1,t2,t3;
    std::vector<int> resultat;

    //pour chaque ligne du fichier
    while (!file.eof()) {
        std::getline(file,tmp);

        //recuperation des points
        if(tmp[0] == 'v' && tmp[1] == ' '){
            ex = explode(tmp,' ');

            //recuperation des valeurs
            std::istringstream streamA(ex[1]);
            streamA >> a;
            std::istringstream streamB(ex[2]);
            streamB >> b;

            a = a*(largeur/2)+(largeur/2);
            b = b*(hauteur/2)+(hauteur/2);

            resultat.push_back(a);
            resultat.push_back(b);
            positions.push_back(resultat);
            resultat.clear();
        }
        //recuperations des triangles a tracer
        if(tmp[0] == 'f' && tmp[1] == ' '){
            ex = explode(tmp,' ');
            std::istringstream streamT1(explode(ex[1],'/')[0]);
            streamT1 >> t1;
            std::istringstream streamT2(explode(ex[2],'/')[0]);
            streamT2 >> t2;
            std::istringstream streamT3(explode(ex[3],'/')[0]);
            streamT3 >> t3;
            resultat.push_back(t1);
            resultat.push_back(t2);
            resultat.push_back(t3);
            triangles.push_back(resultat);
            resultat.clear();
        }
    }
    file.close();
}

void affichagePoint(TGAImage &image){
    //affichage des points
    for(int i =0;i < positions.size();i++){
        image.set(positions[i][0],positions[i][1],white);
    }
}

void affichageLignes(TGAImage &image){
    //dessin des lignes
    int d,b,c;
    for(int i = 0;i < triangles.size();i++){
        d = triangles[i][0]-1;
        b = triangles[i][1]-1;
        c = triangles[i][2]-1;
        drawLine(positions[d][0],positions[d][1],positions[b][0],positions[b][1],image,white);
        drawLine(positions[d][0],positions[d][1],positions[c][0],positions[c][1],image,white);
        drawLine(positions[b][0],positions[b][1],positions[c][0],positions[c][1],image,white);
    }
}


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    TGAImage image(largeur, hauteur, TGAImage::RGB);

    lectureFichier("../african_head.obj",image);

    affichageLignes(image);

    image.flip_vertically();
    image.write_tga_file("output.tga");
    return 0;

}
