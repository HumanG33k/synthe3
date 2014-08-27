#ifndef __SYN_PARLE_H_
#define __SYN_PARLE_H_

//Structures
struct Marq {
	bool DSDecroit;	//d�but sous-diphone d�croit
	bool DSCroit;	//d�but sous-diphone croit
	bool FSDecroit;	//fin sous-diphone d�croit
	bool FSCroit;	//fin sous-diphone croit
};

struct UChar {
	unsigned char DSDecroit;	//d�but sous-diphone d�croit
	unsigned char DSCroit;	//d�but sous-diphone croit
	unsigned char FSDecroit;	//fin sous-diphone d�croit
	unsigned char FSCroit;	//fin sous-diphone croit
};

struct PUChar {
	unsigned char* DSDecroit;	//d�but sous-diphone d�croit
	unsigned char* DSCroit;	//d�but sous-diphone croit
	unsigned char* FSDecroit;	//fin sous-diphone d�croit
	unsigned char* FSCroit;	//fin sous-diphone croit
};

struct PSChar {
	char* DSDecroit;	//d�but sous-diphone d�croit
	char* DSCroit;	//d�but sous-diphone croit
	char* FSDecroit;	//fin sous-diphone d�croit
	char* FSCroit;	//fin sous-diphone croit
};

//////////////////////
// classe Parle
//////////////////////
class Parle {
private:
	float mulHauteur;
	float mulDebit;
	float mulVolume;
	short sortieSon;
	short sortieWave;
	char* texPhon;
	float xEcrEchelleDeLec;	//position �criture � l'�chelle de la lecture pour comparaison
	float allonge;	//allongement du ':'
	long iLecPhon;	//indice lecture chaine phon�tique
	long iLecPhonDecroit;	//id selon le point de vue
	long iLecPhonCroit;	//id selon le point de vue
	char phonGDecroit;	//1er phon�me du diphone d�croit
	char phonGCroit;	//1er phon�me du diphone croit
	char phonDDecroit;	//2�me phon�me du diphone d�croit
	char phonDCroit;	//2�me phon�me du diphone croit
	char phonMem;	//m�moris�
	char catGDecroit;	//cat�gorie de phonGDecroit
	char catGCroit;	//cat�gorie de phonGCroit
	char catDDecroit;	//cat�gorie de phonDDecroit
	char catDCroit;	//cat�gorie de phonDCroit
	char nSousDiphDecroit;	//0, 1, 2 n� de sous-diphone d�croit
	char nSousDiphCroit;	//0, 1, 2 n� de sous-diphone croit
	Marq marq;	//pr�sence d'un sous-diphone dans le m�lange
	UChar nSeg;	//n� de segment
	UChar nSegIni;	//nSeg initial
	PSChar ptSeg;	//pt sur segment
	UChar perio;	//p�riode
	float perioBase;	//p�riode avant action de la hauteur
	unsigned char perioResult;		// p�riode r�sultante
	PUChar ptAmp;	//pt sur amplitudes
	UChar amp;	//amplitude
	UChar ampAnc;	//amplitude pr�c�dente (pour interpolation lin�aire)
	short iEch;	//�chantillon dans la p�riode

public:
	void traiteTextePhonetique(char* chainePhon);

private:
	//Sort une p�riode du signal
	bool traiteUnePeriode();
	//Initialise tous les D�croit sur les Croit
	void initDecroitSurCroit();
	//Pr�pare les variables de la p�riode suivante
	bool perioSuiv(bool& marqDS, bool& MarqFS, long& iLecPhonX, char& phonG, char& phonD, char& catG, char& catD,
		char& nSousDiph, unsigned char& nSegIniDS, unsigned char& nSegIniFS,
		unsigned char& nSegDS, unsigned char& nSegFS, char*& ptSegDS, char*& ptSegFS,
		unsigned char& perioDS, unsigned char& perioFS,
		unsigned char*& ptAmpDS, unsigned char*& ptAmpFS,
		unsigned char& ampDS, unsigned char& ampFS, bool mCroit);
	//Pr�pare les variables du sous-diphone d�croit suivant
	bool nouveauSousDiph(long& iLecPhonX, char& phonG, char& phonD, char& catG, char& catD, char& nSousDiph,
		unsigned char& nSegIniDS, unsigned char& nSegDS, unsigned char*& ptAmpDS,
		unsigned char& ampDS);
	//Phon�me suivant dans le texte phon
	char phonSuiv(long& iLecPhonX, char phonG);
	//Calcule la part croissante de la p�riode
	short calculeEchPerioCroit(short x, char* ptSegDecroit, unsigned char perioDecroit, unsigned char ampDecroit);
	//Calcule la part d�croissante de la p�riode
	short calculeEchPerioDecroit(short x, char* ptSegCroit, unsigned char perioCroit, unsigned char ampCroit);
};

#endif