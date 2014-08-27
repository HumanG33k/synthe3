///////////////////////////////////////////////////////
// *** SYNTHE ***
// Synth� logiciel (fonctions publiques)
///////////////////////////////////////////////////////

#ifdef WIN32
	#define EXTERNE extern "C" _declspec(dllexport)	//pour Synthe.h
#else // linux ...
	#define EXTERNE extern "C"
#include <unistd.h>
	#include <pthread.h>
#endif

#define SYNTHE_VERSION "Synth� version 1.1"
//#define SYNTHE_VOIX "" //Chemin et nom du fichier de voix
//#define SYNTHE_TAB "" //Chemin et nom du fichier des tables

#include <ctime>
#include <string.h>
#include "SynMain.h"
#include "Synthe.h"
#include "SynVoix.h"

//Variables de Synthe.cpp
HANDLE hThread;
short posLec[NM_INDEX+1];	//il y a une place de plus que d'index
short nbIndex;
Voix** tVoix;	//tableau des voix
Tab* tab;	//les tables de Synth� (objet, ne pas confondre avec tTab, table qui fait partie de cet objet)

//////////////////////////////////////////////////////////////////
// Fonctions publiques pour faire parler Synth�
//////////////////////////////////////////////////////////////////

//Les fonctions de Synth� sont en mode _stdcall pour les appels � partir d'autres langages (Visual Basic, ...)
//Le nombre d'index est calcul� avant de cr�er le thread, pour �tre bon d�s le d�but (min 1 pendant le son)

//Envoi d'un texte � lire par Synth�
//Le param�tre texte est obligatoire, les autres sont facultatifs, la valeur -1 indique la conservation de la valeur courante
void _stdcall synTexte(
	char* texte,	//peut �tre constitu� de plusieurs paragraphes, sans d�passer NM_CAR_TEX caract�res.
	short volume,	//0 � 15 par pas de 25 % (par d�faut 10)
	short debit,	//0 � 15 par pas de 12 % (par d�faut 4)
	short hauteur,	//0 � 15 par pas de 12 % (par d�faut 4)
	short phon, 	//1, le texte est phon�tique
	short modeLecture,	//0, normal, 1, dit la ponctuation
	short modeCompta,	//0, le s�parateur de milliers reste, 1, le s�prateur de milliers est enlev�
	short sortieSon,		//sortie sur la carte-son
	short sortieWave,		//sortie sous forme de fichier wave
	char* nomFicWave	//nom �ventuel du fichier � construire
	) {
	
	typeParamThread* lpParamThread=new typeParamThread;
#ifdef WIN32
	DWORD idThread;
#endif
	char* bufferMessage;
	long longTex;	//longueur texte

	demandeStopEtAttendFinThread();	//stop texte en cours
	if (!texte) {
		delete lpParamThread;
		return;
	}
	longTex=strlen((char*)texte);
	if (longTex>NM_CAR_TEX) longTex=NM_CAR_TEX;
	bufferMessage=new char[longTex*2+10];	//au pire : "a b c d ..."
	//Demande la version de Synth�
	if (strncmp(texte,"nversionsyn",9)==0) strcpy (bufferMessage, SYNTHE_VERSION);
	copieEtAjouteIndexSiPas(texte, bufferMessage);	//compte les index
	//Cr�e le thread
	lpParamThread->texte=bufferMessage;
	lpParamThread->phon=phon;
	lpParamThread->volume=volume;
	lpParamThread->debit=debit;
	lpParamThread->hauteur=hauteur;
	lpParamThread->modeLecture=modeLecture;
	lpParamThread->modeCompta=modeCompta;
	lpParamThread->sortieSon=sortieSon;
	lpParamThread->sortieWave=sortieWave;
	lpParamThread->nomFicWave=nomFicWave;
	synGlobal.setThreadOK(false);
#ifdef WIN32
	hThread=CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&fThAlpha, lpParamThread, 0, &idThread);
	SetPriorityClass(hThread, HIGH_PRIORITY_CLASS);
	Sleep(0);	//acc�l�re la cr�ation du thread
#else
	pthread_create(&hThread, NULL, (void*(*)(void*))fThAlpha, lpParamThread);
	sleep(0);	//acc�l�re la cr�ation du thread
#endif
}

//Retourne la valeur de l'index de lecture (va du nb d'index � 0 en fin de lecture)
short synIndex() {
	return synGlobal.getNbIndexLec();
}

//Retourne la position de lecture du texte
short _stdcall synPosLec() {
	short n=synGlobal.getNbIndexLec();
	if (n<1) return -1;
	return posLec[nbIndex-n];
}

//Stop parole : arr�te la lecture (effet imm�diat)
//Indispensable quand on arr�te un programme utilisant Synth� avec DirectX
//	sous peine de bouclage irr�versible du tampon de lecture
void _stdcall synStop() {
	demandeStopEtAttendFinThread();
}

/////////////////////////////
//	Fonctions priv�es
/////////////////////////////

//D�marrage du thread : synth�se � partir du texte
void fThAlpha(void* lpParam) {
	synTex(lpParam);
}

//Si thread en cours : demande stop puis attend la fin du thread
void demandeStopEtAttendFinThread() {
#ifdef WIN32
	if (WaitForSingleObject(hThread,0)==WAIT_TIMEOUT) {
		synGlobal.setDemandeStop(true);	//demande le stop
		WaitForSingleObject(hThread,INFINITE); //et attend la fin du Thread
		synGlobal.setDemandeStop(false);	//OK, on peut envoyer un nouveau message
	}
#else
	if (hThread) {
		synGlobal.setDemandeStop(true);	//demande le stop (positionne une globale)
		//pthread_cancel(hThread);
		pthread_join(hThread, NULL);
		synGlobal.setDemandeStop(false);	//OK, on peut envoyer un nouveau message
	}
#endif
	synGlobal.setNbIndexLec(-1);	//-1 -> 0 indique lecture termin�e (� 0 -> 1, il reste ce qui suit le dernier index)
	initWave(false);	//termine une �ventuelle sortie wave
	sonDestruction();	//d�truit l'objet son synSon (donc le buffer circulaire et l'objet carte-son snd_dev)
}

//Indexation automatique : copie la chaine en ajoutant des index si elle n'en comporte pas (et les compte)
void copieEtAjouteIndexSiPas (char* chaineLec, char* chaineEcr) {
	char* lec;
	char* ecr=chaineEcr;
	bool marqValide=false;

	posLec[0]=0;	//position initiale
	nbIndex=1;	//init comptage � 1 pour posLec car 0 repr�sente le 1er mot (mais on d�cr�mente apr�s)
	for (lec=chaineLec; *lec!=0; lec++) {
		if (*lec=='�') {
			if (lec[1]=='�') {	//index trouv�
				if (nbIndex<=NM_INDEX)
					posLec[nbIndex++]=lec-chaineLec;	//rep�re l'index et compte
				lec++;
			}
		}
	}
	if (nbIndex>1) {
		//La chaine comporte des index
		ecr=chaineEcr;
		for (lec=chaineLec; *lec!=0 && lec<chaineLec+NM_CAR_TEX; lec++)	//limite la lecture � NM_CAR_TEX carac
			*ecr++=*lec;	//recopie simplement
		*ecr=0;
		synGlobal.setNbIndexLec(nbIndex-1);	//valeur pour Synth� avant mise � jour par lecture tampon
		synGlobal.setNbIndexMax(nbIndex-1);	//pour index sous linux
		return;
	}
	//Pas d'index : on les place
	ecr=chaineEcr;
	for (lec=chaineLec; *lec!=0 && lec<chaineLec+NM_CAR_TEX; lec++) {	//limite la lecture � NM_CAR_TEX carac
		if (*lec==0) {
			break;	//si finit par marqueur
		}
		if (!caracValide((char)*lec)) {
			if (marqValide) {	//place l'index derri�re le car valide et le rep�re
				if (nbIndex<NM_INDEX) {
					posLec[nbIndex++]=lec-chaineLec;
					*ecr++=MARQ_MARQ; *ecr++=MARQ_INDEX;
				}
			}
			marqValide=false;
		} else
		  marqValide=true;  //pr�t � placer le prochain index
		*ecr++=*lec;
	}
	*ecr++=MARQ_MARQ; *ecr++=MARQ_INDEX;*ecr=0;	//finit par un index
	posLec[nbIndex]=lec-chaineLec;	//position du dernier index (inutile car il sera ignor� et la vraie fin de lecture renverra -1)
	synGlobal.setNbIndexEcr(nbIndex);	//� �crire dans le tampon circulaire (min 1 pendant le son)
	synGlobal.setNbIndexLec(nbIndex);	//� retourner avant mise � jour par lecture tampon (min 1 pendant le son)
	synGlobal.setNbIndexMax(nbIndex);	//Romain pour index sous linux
}

//D�termine s'il s'agit d'un caract�re ou d'un s�parateur
bool caracValide(char carac) {
	if (carac==' ' || carac==10 || carac==13) return false;	//on pourrait affiner avec qques carac ou un tableau
	return true;
}

////////////////////////////////////
// Initialisation de Synth�
////////////////////////////////////

//Au lancement
void initSynthe() {
	initVariablesSectionCritiqueGlobal();
	synGlobal.initTNEch(NM_INDEX);	//pour index sous Linux
	synGlobal.setNbIndexLec(0);
	synGlobal.setDemandeStop(false);
	synGlobal.setVolume(10);
	synGlobal.setDebit(4);
	synGlobal.setHauteur(6);
	synGlobal.setPhon(0);
	synGlobal.setModeLecture(0);
	synGlobal.setModeCompta(1);
	synGlobal.setSortieSon(1);
	synGlobal.setSortieWave(0);
	tVoix=new Voix*[1];	//une seule voix (pr�vu pour plusieurs)
	tVoix[0]=new Voix(0, "Michel.seg");	//construit la voix 0 (la seule)
	tab=new Tab("Synthe.tab");	//construit les tables
	//		synTexte("synth� pr�t");
}

//Pour quitter
void quitteSynthe() {
	demandeStopEtAttendFinThread();	//stoppe le message en cours, index � -1, d�truit l'objet son synSon, donc le buffer circulaire et l'objet carte-son snd-dev
	synGlobal.destTNEch();	//index sous Linux
	synGlobal.setNomFichierWave(NULL);
	detruitVariablesSectionCritiqueGlobal();
	delete tab;
	delete tVoix[0];
	delete[] tVoix;
}

//Initialisation chargement/d�chargement la dll (pour Linux, il faut appeler initSynthe et quitteSynthe
#ifdef WIN32
BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, void* lpReserved) {
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		initSynthe();
		break;
	case DLL_PROCESS_DETACH:
		quitteSynthe();
		break;
	}
	return true;
}
#endif
