/////////////////////////////////////////////////////
//	Synth� : fonctions math�matiques
/////////////////////////////////////////////////////

#include <math.h>
#include "SynCalcul.h"

//Fen�tre croissante (sinuso�dale), position x, entre 0 et 1
float fenCroit(float x) {
	if (x<0) x=0;
	else if (x>1) x=1;
	return (1-(float)cos(x*PI))/2;
}

//Fen�tre d�croissante (sinuso�dale), position x, entre 1 et 0
float fenDecroit(float x) {
	if (x<0) x=0;
	else if (x>1) x=1;
	return (1+(float)cos(x*PI))/2;
}

