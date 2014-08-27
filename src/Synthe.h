#ifndef ___SYNTHE_H__
#define ___SYNTHE_H__

#ifndef EXTERNE
	#ifdef WIN32
		#define EXTERNE extern "C" _declspec(dllimport)
	#else // linux ...
		#define EXTERNE extern "C"
	#endif
#endif

#ifndef WIN32
	#define _stdcall
#endif

//////////////////////////////////////////////////////////////////
// Fonctions publiques pour faire parler Synth�
//////////////////////////////////////////////////////////////////

//Envoi d'un texte � lire par Synth�
//Le param�tre texte est obligatoire, les autres sont facultatifs, la valeur -1 indique la conservation de la valeur courante
EXTERNE void _stdcall synTexte(
	char* texte,	//peut �tre constitu� de plusieurs paragraphes, sans d�passer NM_CAR_TEX caract�res.
	short volume=-1,	//0 � 15 par pas de 25 % (par d�faut 10) (-1 indique inchang�)
	short debit=-1,	//0 � 15 par pas de 12 % (par d�faut 4)
	short hauteur=-1,	//0 � 15 par pas de 12 % (par d�faut 4)
	short phon=-1, 	//1, le texte est phon�tique
	short modeLec=-1,	//0 � 15 par pas de 6 % (par d�faut 6)
	short compta=-1,	//0, le s�parateur de milliers reste, 1, le s�prateur de milliers est enlev�
	short son=-1,		//sortie sur la carte-son
	short wave=-1,		//1, sortie sous forme de fichier wave
	char* nomWave=NULL	//nom �ventuel du fichier � construire
	);

//Retourne la valeur de l'index de lecture (va du nb d'index � 0 en fin de lecture)
EXTERNE short synIndex();

//Retourne la position de lecture du texte (indice du caract�re en cours de lecture)
EXTERNE short _stdcall synPosLec();

//Stop parole : arr�te la lecture (effet imm�diat)
//Indispensable quand on arr�te un programme utilisant Synth� sous peine de bouclage irr�versible du tampon de lecture
EXTERNE void _stdcall synStop();

//Initialisation de Synthe
void initSynthe();	//init section critique, r�glages, voix, tables (au d�marrage de l'utilisation de Synth�)
void quitteSynthe();	//stoppe parole, d�truit tout (n�cessaire pour quitter Synth� avant d'arr�ter l'application)

#endif
