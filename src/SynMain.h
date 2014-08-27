#ifndef __SYN_MAIN_H__
#define __SYN_MAIN_H__

//D�clarations communes aux fichiers de Synth�

#include "SynGlobal.h"
#ifdef WIN32
	#include <windows.h>	//n�cessaire pour Synth�
#else
	#define LPVOID void*
	#define DWORD int
	#define HANDLE pthread_t
#endif
 
//Constantes
//Taille max texte
#define NM_CAR_TEX 1000
#define NM_CAR_TEX_1 NM_CAR_TEX-1
#define NM_CAR_TEX_2 NM_CAR_TEX-2
#define NM_CAR_TEX_8 NM_CAR_TEX-8
//Constantes pour conversion alpha-phon�mes
#define BVRD 1	//teste le mode bavard
#define NABR 2	//teste le mode non abr�viations
//Cat�gories (attention : ds l'arbre la cat�gorie est test�e par < ou =)
#define VOYM 3	//voyelle "mouill�e"
#define VOY 4	//voyelle
#define LET 5	//lettre
#define CNS 5	//lettre
#define PONC 6	//ponctuation
#define SYMB 7	//symbole
#define CHIF 8	//chiffre
#define TTT 8	//tout
//Terminateurs pour arbre (alpha-phon�mes)
#define PG 80	//programme (retour au cas g�n�ral)
#define RB 81	//arbre
#define DC 82	//dictionnaire
//Cat�gories phon�tiques
#define VOYP 0	//voyelles
#define GLIP 1	//glissantes
#define CNSP 2	//consonnes
//Phon�mes
#define OU 0
#define WW 0
#define OO 1
#define UU 2
#define II 3
#define AA 4
#define AN 5
#define ON 6
#define EU 7
#define EE 8
#define UN 9
#define YY 10
#define FF 11
#define SS 12
#define HH 13
#define VV 14
#define ZZ 15
#define JJ 16
#define PP 17
#define TT 18
#define KK 19
#define BB 20
#define DD 21
#define GG 22
#define MM 23
#define NN 24
#define LL 25
#define RR 26
#define VR 27	//virgule phon
#define PL 28	//allongement
#define INDEX '_'
#define NULP -2	//nul phon

//Tableaux n�cessaire pour pouvoir retrouver les valeurs d�finies � l'ext�rieur du thread
extern HANDLE hThread;
extern short posLec[NM_INDEX+1];	//il y a une place de plus que d'index
extern short nbIndex;

//Structures
struct typeParamThread {
	char* texte;	//peut �tre constitu� de plusieurs paragraphes, sans d�passer NM_CAR_TEX caract�res.
	short phon; 	//1, le texte est phon�tique
	short volume;	//0 � 15 par pas de 25 % (par d�faut 10)
	short debit;	//0 � 15 par pas de 12 % (par d�faut 4)
	short hauteur;	//0 � 15 par pas de 12 % (par d�faut 4)
	short modeLecture;	//0, normal, 1, dit la ponctuation
	short modeCompta;	//0, le s�parateur de milliers reste, 1, le s�prateur de milliers est enlev�
	short sortieSon;	//sortie sur la carte-son
	short sortieWave;		//sortie sous forme de fichier wave
	char* nomFicWave;	//nom �ventuel du fichier � construire
};

//Fonctions externes de SynTex.cpp
extern void synTex(void* lpParam);

//Fonctions externes de SynParle.cpp
extern void initWave(bool init);
extern void sonDestruction();

//Fonctions priv�es de Synthe.cpp
void fThAlpha(void*);
void copieEtAjouteIndexSiPas (char* chaineLec, char* chaineEcr);
bool caracValide(char carac);
void demandeStopEtAttendFinThread();

#endif
