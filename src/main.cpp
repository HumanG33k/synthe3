#include <stdio.h>
#include <stdlib.h>
#include "Synthe.h"

int main( int argc, char** argv )
{
	initSynthe();

	//Prononce une phrase avec les param�tres par d�faut (� -1) : volume, d�bit, hauteur, phon�tique, modeLec, compta
	//puis 0 (pas de son) puis 1 (sortie wave), puis "essai.wav"
	//	synTexte("Ceci est une phrase que je prononce correctement.", -1, -1, -1, -1, -1, -1, 0, 1, "/tmp/essaiSynthe1.wav");
		synTexte("Ceci est une phrase que je prononce correctement.", -1, -1, -1, -1, -1, -1, 0, 1);
	while(synIndex()>0);	//attend la fin de la phrase (sinon coupe)
	/*	synTexte("Et la suivante.", -1, -1, -1, -1, -1, -1, 0, 1, "/tmp/essaiSynthe2.wav");
		while(synIndex()>0);	//attend la fin de la phrase (sinon coupe)
	synTexte("� l'�cole, les �l�ves �coutent la le�on donn�e par le ma�tre.", -1, -1, -1, -1, -1, -1, 0, 1, "/tmp/essaiSynthe3.wav");
	while(synIndex()>0);	//attend la fin de la phrase (sinon coupe)
	*/
		
	quitteSynthe();

	exit(0);
}
