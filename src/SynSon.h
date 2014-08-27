#ifndef __SYN_SON_H_
#define __SYN_SON_H_

////////////////////////////////////
// Sortie vers la carte-son
////////////////////////////////////

#ifdef WIN32
	#define CARTESON LPDIRECTSOUND
	#include <dsound.h>
#else
	#define LPVOID void*
	#define DWORD int
	#define Sleep sleep
	#include <sys/time.h>
	#ifdef ALSA
		#define CARTESON snd_pcm_t*
		#define ALSA_PCM_NEW_HW_PARAMS_API
		#include <alsa/asoundlib.h>
	#else // OSS
		#define CARTESON int
		#include <fcntl.h>
		#include <sys/ioctl.h>
		#include <linux/soundcard.h>
	#endif
#endif

#include <cstdio>

const double UNITE_TEMPS=1./11025;	//unit� de temps repr�sentant 1 ou 2 �chantillons
const double TEMPS_PAQUET=.02;	//20 ms par paquet
//const double TEMPS_PAQUET=.1;	//100 ms par paquet
const double TEMPS_SECU=.2;	//200 ms pour incertitude position
const double TEMPS_TAMPON=1;	//1000 ms pour le tampon secondaire

class classSon {

protected :
#ifdef WIN32 // partie d�di�e DirectSound
	LPDIRECTSOUNDBUFFER lpBuffer; // buffer circulaire DirectSound (secondaire)
	LPDWORD ptIOctLecDS; // pointeur sur position de lecture du buffer direct sound (en octets)
	LPDWORD ptIOctEcrDS; // pointeur sur position d'�criture du buffer direct sound (en octets, non exploit�)
	HWND Handle; // Handle de la fen�tre courante
	long DSVolume;	// volume du buffer DirectSound
#endif
	CARTESON snd_dev;       // id de la carte son (DirectSound, ALSA et OSS)
	long nbEchLpBuffer;	//nb �ch dans buffer secondaire
	long nbEchLpBuffer2;	//idem / 2
	char* paquet8;	// paquet de mots 8 bits
	short* paquet16;	// paquet de mots 16 bits
	short* tIndex;		// table des index
	short nbVoiesSon;	// nb voies : 1 mono, 2 st�r�o
	long nbEchPaquet;	//nb �ch par paquet
	long nbEchSecu;	//nb �ch pour incertitude position lecture
	long nbOctetsPaquet;	//taille en octets du paquet
	short nbPaquetsLpBuffer;
	short nbBitsParEchVoie;	// 8 ou 16 bits
	short nbOctetsParEch;	//1 � 4
	int fEchCarte;          // fr�quence d'�chantillonnage de la carte son
	short indexAEcrire;     // index � �crire dans le tableau tIndex (en d�but de paquet)
	long iEchEcr;	// position d'�criture dans le buffer circulaire "lpBuffer"
	long iEchPaquet;	// position dans le paquet
	long iEchPosLec;	//position de lecture dans le buffer circulaire "lpBuffer"
	long iEchPosLecAnc;	//id val pr�c�dente
	long iEchPosLecAv;	//variante pour finir son
	bool marqJoue;	// sortie du son en cours
	bool marqJoueInit;	//sortie du son initialis�e et commenc�e
	bool pauseEnCours;	// son en pause
	short etatFin;	//0=normal, 1=lire derri�re la limite, 2=lire jusqu'� la limite
	long limLit;	//=iEchEcr final du message
	long limLitPaquet;	//=iEchPaquet final du message
	bool snd_ok;	// carte son ouverte avec succ�s

private :
	long echTot;
	short nbTot;
	long cumul;

public :
	classSon(int frequence, short typeEchVoie); 
	~classSon();
	bool ouvertOK();
	bool transfert(LPVOID);
	void sonExit();	//fin du message : termine le buffer proprement
	void finirSon(bool marqFin);
	bool pauseSiJoue();	//stoppe le son sous DirectSound
	bool joueSiPause();	//d�marre le son sous DirectSound
	void attendSiEcrOuLecRattrape();
	void positionLecture();
#ifdef WIN32
	bool initWindow();
#endif
	static char *yes_no(int condition) {
		if (condition) return "yes"; else return "no";
	}
private:
#ifdef WIN32
	bool transferePaquet(LPDIRECTSOUNDBUFFER, DWORD, LPVOID, DWORD);
	HRESULT creeBufferSecondaire(LPDIRECTSOUNDBUFFER*);
	bool modifieBufferPrimaire();
#else
	bool transferePaquet(LPVOID, DWORD);
#endif
	void open_snd();
	void close_snd();
	void get_snd_params();
	void set_snd_params(int, int, int);
};

#endif
