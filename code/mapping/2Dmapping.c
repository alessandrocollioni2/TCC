/*
 ******************************************************************************
 *                          UNIVERSIDADE DE SAO PAULO                         *
 *             INSTITUTO DE CIENCIAS MATEMATICAS E DE COMPUTACAO              *
 *----------------------------------------------------------------------------*
 * DESENVOLVIDO POR: HEITOR DE FREITAS VIEIRA                DATA: 07/03/2009 *
 *                                                                            *
 * DEPENDENCIAS: PLAYER, STAGE e OPENCV                                       *
 *                                                                            *
 * PODE SER ALTERADO E DISTIBUIDO LIVREMENTE, DESDE QUE MENCIONADO O AUTOR.   *
 * TESTADO NO UBUNTU 8.10                                                     *
 *                                                                            *
 * Para compilar:                                                             *
 * gcc 2Dmapping.c -o 2Dmapping -I/usr/local/include/player-2.1 -L/usr/local/lib -lplayerc -lm -lplayerxdr -lplayererror -lcv -lhighgui -lcxcore -ljpeg
 ******************************************************************************
 */

#include <stdio.h>
#include <libplayerc/playerc.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <math.h>

#define PI 3.14159265

//Converte angulos em grau para radianos
float graus2rad (int graus) {
   float temp = ((graus*PI)/180);
   return temp;
}

//Transforma ponto flutuante em inteiro e arredonda
int arredonda (float num) {
    int temp = (int)(num*100);
    temp = temp%10;
    if (temp >= 5) return (int)((num*10)+1);
    else return (int)(num*10);
}

int main(int argc, const char **argv)
{
    //Variaveis
    int degrees,PosRelX,PosRelY;
    float radians,Dlaser,ODM_ang, ang;
    int width = 500, height = 500; //Coloque o tamanho do mapa aqui (em pixel)
    int centroX = (width / 2);
    int centroY = (height / 2);
    playerc_client_t *client;
    playerc_laser_t *laser;
    playerc_position2d_t *position2d;
    CvPoint pt,pt1,pt2;
    CvScalar cinzaE,preto,cinzaC;
    char window_name[] = "Mapa";

    IplImage* image = cvCreateImage( cvSize(width,height), 8, 3 );
    cvNamedWindow(window_name, 1 );
    preto = CV_RGB(0, 0, 0);        //Para indicar obstaculos
    cinzaE = CV_RGB(92, 92, 92);    //Para indicar o desconhecido
    cinzaC = CV_RGB(150, 150, 150); //Para indicar espacos livres

    client = playerc_client_create(NULL, "localhost", 6665);
    if (playerc_client_connect(client) != 0)
    return -1;

    laser = playerc_laser_create(client, 0);
    if (playerc_laser_subscribe(laser, PLAYERC_OPEN_MODE))
    return -1;

    position2d = playerc_position2d_create(client, 0);
    if (playerc_position2d_subscribe(position2d, PLAYERC_OPEN_MODE) != 0) {
        fprintf(stderr, "error: %s\n", playerc_error_str());
        return -1;
    }

    if (playerc_client_datamode (client, PLAYERC_DATAMODE_PULL) != 0) {
        fprintf(stderr, "error: %s\n", playerc_error_str());
        return -1;
    }

    if (playerc_client_set_replace_rule (client, -1, -1, PLAYER_MSGTYPE_DATA, -1, 1) != 0) {
        fprintf(stderr, "error: %s\n", playerc_error_str());
        return -1;
    }

    playerc_position2d_enable(position2d, 1);  // Liga os motores
    playerc_position2d_set_odom(position2d, 0, 0, 0);  // Zera o odômetro

    cvSet(image, cinzaE,0); //Preencha a imagem com fundo cinza escuro
    pt.x = centroX;  // Zera a coordenada X
    pt.y = centroY;  // Zera a coordenada Y


    while(1) {
        playerc_client_read(client);
        cvSaveImage("mapa.jpg",image);
        playerc_client_read(client);

        for (degrees = 2; degrees <= 360; degrees+=2) {
            Dlaser = laser->scan[degrees][0];
            if (Dlaser < 8) {
                radians = graus2rad (degrees/2);      //Converte o angulo do laser em graus para radianos
                ODM_ang = position2d->pa;             //Obtem o angulo relativo do robo
                ang = ((1.5*PI)+radians+ODM_ang);     //Converte o angulo relativo em global
                PosRelX = arredonda(position2d->px);  //Posicao X relativa do robo
                PosRelY = arredonda(position2d->py);  //Posicao Y relativa do robo
                pt1.y = (centroY-PosRelY);            //Coordenada y global do robo
                pt1.x = (centroX+PosRelX);            //Coordenada x global do robo

                //converte coordenadas polares para retangulares (global)
                pt.y = (int)(pt1.y-(sin(ang)*Dlaser*10));
                pt.x = (int)(pt1.x+(cos(ang)*Dlaser*10));

                //Desenha a area livre
                cvLine(image, pt1,pt,cinzaC, 1,4,0);

                //Marca o objeto no mapa
                cvLine(image, pt,pt,preto, 1,4,0);

                //Mostra o resultado do mapeamento na tela
                cvShowImage(window_name, image );
                cvWaitKey(10);
            }
        }
    }

    //Desconecta o player
    playerc_laser_unsubscribe(laser);
    playerc_laser_destroy(laser);
    playerc_client_disconnect(client);
    playerc_client_destroy(client);

    //Destroi a janela OpenCV
    cvReleaseImage(&image);
    cvDestroyWindow(window_name);
    return 0;
}



