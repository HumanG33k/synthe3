#include "SynVoix.h"
#include "SynMain.h"

///////////////////////////////////////////
// M�thodes de la classe Voix
///////////////////////////////////////////

Voix::Voix(char nVoix, char* nomFicVoix) {	//constructeur : charge la voix
	//Charge voix Michel
	ifstream ficVoix(nomFicVoix, ios::binary);
	ficVoix.read((char*)&nbSeg, 2);
	ficVoix.read((char*)&nbEch, 2);
	tAdr=new short[nbSeg];
	tSeg=new char[nbEch];
	ficVoix.read((char*)tAdr, nbSeg*2);
	ficVoix.read(tSeg, nbEch);
	ficVoix.close ();
}

Voix::~Voix() {	//destructeur
	delete[] tAdr;
	delete[] tSeg;
}

//Retourne un pointeur sur le segment
char* Voix::getPtSeg(unsigned char nSeg) {
	return tSeg+tAdr[nSeg];
}

//////////////////////////////////////////
// M�thodes de la classe Tab
//////////////////////////////////////////

Tab::Tab(char* nomFicTab) {
	//Charge tables
	ifstream ficTab(nomFicTab, ios::binary);
	ficTab.read((char*)&aTWin, 2);
	ficTab.read((char*)&aTCat, 2);
	ficTab.read((char*)&aTPhon1, 2);
	ficTab.read((char*)&aTPhon2, 2);
	ficTab.read((char*)&aTCatP1, 2);
	ficTab.read((char*)&aTCatP2, 2);
	ficTab.read((char*)&aChLst1, 2);
	ficTab.read((char*)&aChLst2, 2);
	ficTab.read((char*)&aChLst3, 2);
	ficTab.read((char*)&aCh01, 2);
	ficTab.read((char*)&aCh1S, 2);
	ficTab.read((char*)&aCh100, 2);
	ficTab.read((char*)&aCh100Z, 2);
	ficTab.read((char*)&aTabVol, 2);
	ficTab.read((char*)&aTabVit, 2);
	ficTab.read((char*)&aTabHau, 2);
	ficTab.read((char*)&aCateg, 2);
	ficTab.read((char*)&aFinAmp, 2);
	ficTab.read((char*)&aFinTim, 2);
	ficTab.read((char*)&aTraAmp, 2);
	ficTab.read((char*)&aTraTim, 2);
	ficTab.read((char*)&aDebAmp, 2);
	ficTab.read((char*)&aDebTim, 2);
	ficTab.read((char*)&aTabAmp, 2);
	ficTab.read((char*)&aAdAmp, 2);
	ficTab.read((char*)&aTabDeb, 2);
	ficTab.read((char*)&a_Huit1, 2);
	ficTab.read((char*)&a_Dix1, 2);
	ficTab.read((char*)&aXChif, 2);
	ficTab.read((char*)&lgTab, 2);
	ficTab.read((char*)&FOIS, 1);
	decal=(short)ficTab.tellg();
	tTab=new char[lgTab];
	ficTab.read(tTab, lgTab);
	ficTab.close ();
	tTab-=decal;	//d�calage liste des adresses
}

Tab::~Tab() {
	tTab+=decal;
	delete[] tTab;
}

//codes Windows (127 � 255) -> code commun
unsigned char Tab::tWin(unsigned char carac) {
	if (carac<32) carac=32;
	if (carac>='a' && carac<='z') carac-=32;
	if (carac<123) return carac;
	return ((unsigned char*)tTab+aTWin)[carac-123];
}

//code commun (32 � 189) -> cat�gorie
char Tab::tCat(unsigned char c) {
	if (c<32 || c>FOIS) return PONC;
	return (tTab+aTCat)[c-32];
}

//code commun (97 � 122)(FOIS-30 � FOIS-8) -> code phon�tique
char Tab::tPhon(unsigned char c) {
	if (c==',') return VR;
	if (c==':') return PL;
	if (c<65) return NULP;
	if (c<91) return (tTab+aTPhon1)[c-65];
	if (c<97) return NULP;
	if (c<123) return (tTab+aTPhon1)[c-97];
	if (c==174) return UU;
	if (c<FOIS-30) return NULP;
	if (c<FOIS-7) return (tTab+aTPhon2)[c-FOIS+30];
	return NULP;
}

//code commun (97 � 122)(FOIS-30 � FOIS-8) -> cat�gorie phon�tique
char Tab::tCatP(unsigned char c) {
	if (c<97) return NULP;
	if (c<123) return (tTab+aTCatP1)[c-97];
	if (c<FOIS-30) return NULP;
	if (c<FOIS-7) return (tTab+aTCatP2)[c-FOIS+30];
	return NULP;
}

//liste des unit�s jusqu'� 16
char* Tab::chListUnit(char c) {
	return tTab+((short*)(tTab+aChLst1))[c-48];
}

//liste des dizaines
char* Tab::chListDiz(char c) {
	return tTab+((short*)(tTab+aChLst2))[c-49];
}

//mille, million, milliard
char* Tab::chListMil(char n) {
	return tTab+((short*)(tTab+aChLst3))[n];
}

//"z�ro"
char* Tab::ch01() {
	return tTab+aCh01;
}

//"�:[n]" ("un" seul)
char* Tab::ch1S() {
	return tTab+aCh1S;
}

//"s�[t]"
char* Tab::ch100() {
	return tTab+aCh100;
}

//"s�:[z]"
char* Tab::ch100Z() {
	return tTab+aCh100Z;
}

//table des volumes : -10 � 5 (pas 1.26 = 2 tons)
short Tab::tabVol(char n) {
	return ((short*)(tTab+aTabVol))[n+10];
}

//table des vitesses : -3 � 12 (pas 1.1225 = 1 ton)
short Tab::tabVit(char n) {
	return ((short*)(tTab+aTabVit))[n+3];
}

//table des hauteurs : -6 � 9 (pas 1.05946 = 1/2 tons)
short Tab::tabHau(char n) {
	return ((short*)(tTab+aTabHau))[n+6];
}

//cat�gories de phon�mes
char Tab::categ(char phon) {
	return (tTab+aCateg)[phon];
}

//n� de courbe d'amplitude de fin de phon�me (d�but de diphone)
unsigned char Tab::finAmp(char catG, char catD) {
	return (tTab+aFinAmp)[catG*10+catD];
}

//n� de segment de fin de phon�me
unsigned char Tab::finTim(char phon) {
	return ((unsigned char*)tTab+aFinTim)[phon];
}

//n� de courbe d'amplitude de la transition
unsigned char Tab::traAmp(char catG, char catD) {
	return (tTab+aTraAmp)[catG*10+catD];
}

//n� de segment de la transition
unsigned char Tab::traTim(char phonG, char phonD) {
	return ((unsigned char*)tTab+aTraTim)[phonG*32+phonD];
}

//n� de courbe d'amplitude de d�but de phon�me (d�but de diphone)
unsigned char Tab::debAmp(char catG, char catD) {
	return (tTab+aDebAmp)[catG*10+catD];
}

//n� de segment de d�but de phon�me
unsigned char Tab::debTim(char phon) {
	return ((unsigned char*)tTab+aDebTim)[phon];
}

//n� courbe d'amplitude -> pt courbe d'amplitude
unsigned char* Tab::getPtAmp(short nAmp) {
	short adr=((short*)(tTab+aAdAmp))[nAmp];
	return (unsigned char*)(tTab+aTabAmp+adr);
}

//pt table des adresses pour d�but de mot (32 � 189)
char* Tab::getPtTabDeb(unsigned char c) {
	short adr=((short*)(tTab+aTabDeb))[c-32];
	return tTab+adr;
}

//Pr�pare adresse suivante (branche)
char* Tab::getPtArbre(char* ptArbre) {
	short adr=*((short*)ptArbre);
	return tTab+adr;
}

//Pr�pare adresse suivante selon carac (racine)
char* Tab::getPtArbre(char* ptArbre, unsigned char carac) {
	short adr=*((short*)ptArbre);
	adr=((short*)(tTab+adr))[carac-32];
	return tTab+adr;
}

//adresse transcription
char* Tab::_Huit1() {
	return tTab+a_Huit1;
}

//adresse transcription
char* Tab::_Dix1() {
	return tTab+a_Dix1;
}

//adresse transcription
char* Tab::XChif() {
	return tTab+aXChif;
}

