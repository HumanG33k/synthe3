#ifndef SYN_VOIX_H_
#define SYN_VOIX_H_

/////////////////////
// Classe Voix
/////////////////////

class Voix {
private:
	char* tSeg;	//tableau des segments concat�n�
	short* tAdr;	//tableau des adresses des segments
	short nbSeg;	//nb de segments
	short nbEch;	//nb d'�chantillons en tout
public:
	Voix(char nVoix, char* nomFicVoix);
	~Voix();
	char* getPtSeg(unsigned char nSeg);	// demande pointeur sur segment
};

extern Voix** tVoix;	//tableau des voix

///////////////////////
// Classe tables
///////////////////////

class Tab {
private:
	char* tTab;	//tableau des tables
	short decal;	//d�calage liste des adresses des tables
	short aTWin;	//codes Windows (127 � 255) -> code commun
	short aTCat;	//code commun (32 � 217) -> cat�gorie
	short aTPhon1;	//code commun (97 � 122) -> code phon�tique
	short aTPhon2;	//code commun (186 � 208) -> code phon�tique
	short aTCatP1;	//code commun (97 � 122) -> cat�gorie phon�tique
	short aTCatP2;	//code commun (186 � 208) -> cat�gorie phon�tique
	short aChLst1;	//liste des unit�s
	short aChLst2;	//liste des centaines
	short aChLst3;	//liste des 10 � 16
	short aChLst4;	//mille, million, milliard
	short aCh01;	//"z�ro"
	short aCh1S;	//"�:[n]" ("un" seul)
	short aCh100;	//"s�[t]"
	short aCh100Z;	//"s�:[z]"
	short aTabVol;	//table des volumes : -10 � 5 (pas 1.26 = 2 tons)
	short aTabVit;	//table des vitesses : -3 � 12 (pas 1.1225 = 1 ton)
	short aTabHau;	//table des hauteurs : -6 � 9 (pas 1.05946 = 1/2 tons)
	short aCateg;	//cat�gories de phon�mes
	short aFinAmp;	//n� de courbe d'amplitude de fin de phon�me (d�but de diphone)
	short aFinTim;	//n� de segment de fin de phon�me
	short aTraAmp;	//n� de courbe d'amplitude de la transition
	short aTraTim;	//n� de segment de la transition
	short aDebAmp;	//n� de courbe d'amplitude de d�but de phon�me (d�but de diphone)
	short aDebTim;	//n� de segment de d�but de phon�me
	short aTabAmp;	//adresse courbe d'amplitude -> n� courbe d'amplitude
	short aAdAmp;	//n� courbe d'amplitude -> adresse courbe d'amplitude
	short aTabDeb;	//table des adresses pour d�but de mot (32 � 217)
	short a_Huit1;	//adresse transcription
	short a_Dix1;	//adresse transcription
	short aXChif;	//adresse transcription
	short lgTab;	//taille totale des tables

public:
	Tab(char* nomFicTab);
	~Tab();
	unsigned char tWin(unsigned char carac);	//codes Windows (127 � 255) -> code commun
	char tCat(unsigned char c);	//code commun (32 � 217) -> cat�gorie
	char tPhon(unsigned char c);	//code commun (97 � 122)(186 � 208) -> code phon�tique
	char tCatP(unsigned char c);	//code commun (97 � 122)(186 � 208) -> cat�gorie phon�tique
	char* chListUnit(char c);	//liste des unit�s jusqu'� 16
	char* chListDiz(char c);	//liste des dizaines
	char* chListMil(char n);	//mille, million, milliard
	char* ch01();	//"z�ro"
	char* ch1S();	//"�:[n]" ("un" seul)
	char* ch100();	//"s�[t]"
	char* ch100Z();	//"s�:[z]"
	short tabVol(char n);	//table des volumes : -10 � 5 (pas 1.26 = 2 tons)
	short tabVit(char n);	//table des vitesses : -3 � 12 (pas 1.1225 = 1 ton)
	short tabHau(char n);	//table des hauteurs : -6 � 9 (pas 1.05946 = 1/2 tons)
	char categ(char phon);	//cat�gories de phon�mes
	unsigned char finAmp(char catG, char catD);	//n� de courbe d'amplitude de fin de phon�me (d�but de diphone)
	unsigned char finTim(char phon);	//n� de segment de fin de phon�me
	unsigned char traAmp(char catG, char catD);	//n� de courbe d'amplitude de la transition
	unsigned char traTim(char phonG, char phonD);	//n� de segment de la transition
	unsigned char debAmp(char catG, char catD);	//n� de courbe d'amplitude de d�but de phon�me (d�but de diphone)
	unsigned char debTim(char phon);	//n� de segment de d�but de phon�me
	unsigned char* getPtAmp(short nAmp);	//n� courbe d'amplitude -> pt courbe d'amplitude
	char* getPtTabDeb(unsigned char c);	//pt table des adresses pour d�but de mot (32 � 189)
	char* getPtArbre(char* ptArbre);	//pr�pare adresse suivante
	char* getPtArbre(char* ptArbre, unsigned char carac);	//pr�pare adresse suivante
	short tabDeb(unsigned char c);	//table des adresses pour d�but de mot (32 � 217)
	char* _Huit1();	//pt transcription
	char* _Dix1();	//pt transcription
	char* XChif();	//pt transcription
	unsigned char FOIS;	//symbole pour "fois"
};

extern Tab* tab;	//les tables de Synth�

#endif
