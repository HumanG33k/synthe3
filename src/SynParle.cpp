///////////////////////////////////
// Lit le texte phon�tique
///////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SynSon.h"
#include "SynMain.h"
#include "SynCalcul.h"
#include "SynParle.h"
#include "SynVoix.h"

////////////////////////////////////////////
// Constantes de synParle.cpp
////////////////////////////////////////////
#define FREQ_ECH 10000
#define TYPE_ECH 16
#define MUL_AMP 8	//pour max moiti� pleine �chelle
#define MS_UNSPEC 0x7ffff000 // magic length pour wave vers stdout

////////////////////////////
//Variables globales
////////////////////////////

extern classSon* synSon;
long longWave;	//longueur du wave
FILE* ficWave;
bool ficWaveFerme;

////////////////////////////////////////////
// Fonctions de la classe Parle
////////////////////////////////////////////

//Fonction principale : prononce le texte phon�tique re�u
void Parle::traiteTextePhonetique(char* chainePhon) {

	sortieWave=synGlobal.getSortieWave();
	sortieSon=synGlobal.getSortieSon();
	mulHauteur=(float)tab->tabHau(synGlobal.getHauteur()-HAUTEUR_REF)/1000;
	mulDebit=(float)tab->tabVit(synGlobal.getDebit()-DEBIT_REF)/1000;
	mulVolume=(float)tab->tabVol(synGlobal.getVolume()-VOLUME_REF)/1000;
	texPhon=chainePhon;
	if (!strcmp(texPhon, "IXIXIZTEESI_RI"))
		strcpy(texPhon, "XDNIZXB[IZ\\IZITS[IZ\\[HR[BVCTMKG]J]SI[VDS_");
	sonDestruction();	//redondance par s�curit�
	if (synGlobal.getSortieSon()) {	//sortie sur la carte-son
		entreSectionCritiqueGlobal2();	// E E E E E E E E E E E E E
		synSon = new classSon(FREQ_ECH, TYPE_ECH);
		quitteSectionCritiqueGlobal2();	// Q Q Q Q Q Q Q Q Q Q Q Q
		if (!synSon->ouvertOK())
			return;
		synSon->finirSon(false);	//=son en cours
	}
	//Init signal de parole
	iLecPhon=0;	//1er caract�re du texte phon�tique
	iLecPhonCroit=iLecPhon;
	phonDCroit=VR;	//le texte commence par une virgule (va passer gauche)
	catDCroit=tab->categ(phonDCroit);
	nSousDiphCroit=2;	//avant le 1er sous-diphone (prochain=0)
	nSegIni.DSCroit=tab->finTim(phonDCroit);	//segment de VR
	nSeg.DSCroit=nSegIni.DSCroit;
	marq.DSCroit=true;	//d�but de sous-diphone activ�
	marq.FSCroit=false;	//pas de fin de sous-diphone
	//Le prochain sous-diphone sera en fait le 1er
	if (!nouveauSousDiph(iLecPhonCroit, phonGCroit, phonDCroit, catGCroit, catDCroit, nSousDiphCroit,
		nSegIni.DSCroit, nSeg.DSCroit, ptAmp.DSCroit, amp.DSCroit))	//fournit ts les DS
		return;
	ptAmp.DSCroit++;	//pr�pare la prochaine p�riode
	//Init D�croit sur Croit
	initDecroitSurCroit();
	amp.DSDecroit=amp.DSCroit;
	amp.FSDecroit=amp.FSCroit=0;	//pas de fin de sous-diphone
	ptSeg.DSCroit=tVoix[0]->getPtSeg(nSeg.DSCroit);	//segment de VR
	perio.DSDecroit=perio.DSCroit=(unsigned char)*ptSeg.DSCroit++;
	ptSeg.DSDecroit=ptSeg.DSCroit;
	perioResult=(unsigned char)(perio.DSCroit/mulHauteur);
	xEcrEchelleDeLec=perioResult/2*mulDebit;	//simule l'�criture (envoi des �ch) � l'�chelle de la lecture
	allonge= -xEcrEchelleDeLec;	//pour l'allongement des voyelles

#ifndef WIN32
	//Gestion de l'index sous linux (juste avant le d�but d'�criture pour une meilleure pr�cision)
	struct timeval tv;
	gettimeofday(&tv, NULL);
	synGlobal.setktime0s(tv.tv_sec);	//temps initial qui servira de rep�re
	synGlobal.setktime0us(tv.tv_usec);	//�s de la s pour la pr�cision
	short i;
	for (i=0; i<synGlobal.getNbIndexMax()+1; i++)	//il y a 1 place de plus que d'inde
		synGlobal.setTNEch(i, 0);	//tout le tableau � 0
	synGlobal.setCtEch(0);	//ainsi que le compteur d'�chantillons
#endif
	
	//Traite p�riode par p�riode : corps du programme
	while (traiteUnePeriode());

	if (synGlobal.getSortieSon()) synSon->sonExit();
}

//Fabrique une p�riode du signal vocal
bool Parle::traiteUnePeriode() {
	short ech;			//�chantillon final � envoyer � la carte son
	short echDecroit;	//�chantillon de la p�riode Decroit
	short echCroit;	//�chantillon de la p�riode Croit
	short echT;
	long echS;
	char ech8;

	//Cherche la p�riode Croit suivante (traitement du d�bit)
	while (true) {
		// Si on trouve la bonne valeur on s'arr�te
		if (xEcrEchelleDeLec<0) break;
		// Sinon on passe � la suivante (xEcrEchelleDeLec diminue de chaque p�riode lue)
		if (!perioSuiv(marq.DSCroit, marq.FSCroit, iLecPhonCroit, phonGCroit, phonDCroit, catGCroit, catDCroit,
			nSousDiphCroit, nSegIni.DSCroit, nSegIni.FSCroit, nSeg.DSCroit, nSeg.FSCroit, ptSeg.DSCroit, ptSeg.FSCroit,
			perio.DSCroit, perio.FSCroit, ptAmp.DSCroit, ptAmp.FSCroit,
			amp.DSCroit, amp.FSCroit, true))
			break;	//fin du texte : on garde la derni�re p�riode valide
	}
	//Les 4 segments (.DSDecroit, .DSCroit, .FSDecroit, .FSCroit) sont pr�ts
	//Calcule chaque �chantillon de la nouvelle p�riode
	for (iEch=0; iEch<perioResult; iEch++) {
//		for (short i=0;i<50;i++) {	//simule une machine plus lente (10 rien 20 un peu, 30 beaucoup, 50 un peu bouclage)
		echDecroit=calculeEchPerioDecroit(iEch, ptSeg.DSDecroit, perio.DSDecroit, amp.DSDecroit)
			+calculeEchPerioDecroit(iEch, ptSeg.FSDecroit, perio.FSDecroit, amp.FSDecroit);
		echCroit=calculeEchPerioCroit(iEch, ptSeg.DSCroit, perio.DSCroit, amp.DSCroit)
			+calculeEchPerioCroit(iEch, ptSeg.FSCroit, perio.FSCroit, amp.FSCroit);
//		}
		echT=echDecroit+echCroit;
		echS=(long)(echT*mulVolume*MUL_AMP+32768.5);	//positif, arrondi sans distorsion
		if (TYPE_ECH==8) {	//8 bits (pour une raison inconnue, le son est de tr�s mauvaise qualit� en 8 bits)
			echS=echS/256-128;
			if (echS>127) ech8=127;
			else if (echS<-128) ech8=-128;
			else ech8=(char)echS ^ 128;	//inversion du bit de fort poids
		} else {	//16 bits
			echS=echS-32768;
			if (echS>32766) ech=32766;
			else if (echS<-32766) ech=-32766;
			else ech=(short)echS;
		}
		if (sortieWave) {
			if (TYPE_ECH==8)
				fwrite(&ech8, sizeof(char), 1, ficWave);
			else
				fwrite(&ech, sizeof(short), 1, ficWave);
			longWave++;
		}
		if (sortieSon) {
			//Transf�re l'�chantillon vers le fichier ou le buffer de sortie de synSon
			if (TYPE_ECH==8) {
				if (!synSon->transfert((void*)&ech8)) return false;	//faux si stop
			}
			else {
				if (!synSon->transfert((void*)&ech)) return false;	//faux si stop
			}
		}
	}
	//Mises � jour pour pr�parer la p�riode suivante
	ampAnc=amp;	//on repartira de l'amplitude actuelle (4 .valeurs)
	//Avance le pointeur simulant l'�criture � l'�chelle de la lecture
	if (allonge>0)
		allonge-=perioResult*mulDebit;	//cas du ':'
	else
		xEcrEchelleDeLec+=perioResult*mulDebit;	//p�riode �crite
	//Pr�pare p�rio d�croit
	if (iLecPhonCroit==iLecPhon) {	//Croit = courant : on s'y r�f�re avant de calculer Decroit (cas g�n�ral)
		initDecroitSurCroit();	//Initialise D�croit sur Croit
		//D�croit est maintenant la p�riode suivante
		if (!perioSuiv(marq.DSDecroit, marq.FSDecroit, iLecPhonDecroit, phonGDecroit, phonDDecroit,
			catGDecroit, catDDecroit, nSousDiphDecroit, nSegIni.DSDecroit, nSegIni.FSDecroit,
			nSeg.DSDecroit, nSeg.FSDecroit, ptSeg.DSDecroit, ptSeg.FSDecroit, perio.DSDecroit, perio.FSDecroit,
			ptAmp.DSDecroit, ptAmp.FSDecroit, amp.DSDecroit, amp.FSDecroit, false))
			return false;	//fin du message phon�tique
	}	//sinon le d�croit est � jour
	return true;
}

//Initialise tous les D�croit sur les Croit
void Parle::initDecroitSurCroit() {
	iLecPhonDecroit=iLecPhonCroit;
	phonDDecroit=phonDCroit;
	phonGDecroit=phonGCroit;
	catDDecroit=catDCroit;
catGDecroit=catGCroit;
	nSousDiphDecroit=nSousDiphCroit;
	marq.DSDecroit=marq.DSCroit;
	marq.FSDecroit=marq.FSCroit;
	ptAmp.DSDecroit=ptAmp.DSCroit;
	ptAmp.FSDecroit=ptAmp.FSCroit;
	nSegIni.DSDecroit=nSegIni.DSCroit;
	nSegIni.FSDecroit=nSegIni.FSCroit;
	nSeg.DSDecroit=nSeg.DSCroit;
	nSeg.FSDecroit=nSeg.FSCroit;
}

//Pr�pare les variables de la p�riode suivante
bool Parle::perioSuiv(bool& marqDS, bool& marqFS, long& iLecPhonX, char& phonG, char& phonD, char& catG, char& catD,
		char& nSousDiph, unsigned char& nSegIniDS, unsigned char& nSegIniFS,
		unsigned char& nSegDS, unsigned char& nSegFS, char*& ptSegDS, char*& ptSegFS,
		unsigned char& perioDS, unsigned char& perioFS,
		unsigned char*& ptAmpDS, unsigned char*& ptAmpFS,
		unsigned char& ampDS, unsigned char& ampFS, bool mCroit) {
	if (!marqDS && !marqFS)
		return false;
	ampDS=ampFS=0;	//par d�faut si pas marq
	if (marqDS) {
		while (marqDS) {
			ampDS=*ptAmpDS;
			if ((ampDS & 128)==128) {	//d�but du sous-phon�me suivant
				ampDS-=128;
				//DS devient FS
				marqFS=marqDS;
				ptAmpFS=ptAmpDS;
				ampFS=ampDS;
				nSegIniFS=nSegIniDS;
				nSegFS=nSegDS;
				ptSegFS=ptSegDS;
				perioFS=perioDS;
				//Nouveau DS
				ampDS=0;	//par defaut si pas marq
				marqDS=nouveauSousDiph(iLecPhonX, phonG, phonD, catG, catD, nSousDiph,
							nSegIniDS, nSegDS, ptAmpDS, ampDS);	//fournit ts les DS
			} else break;
		}
		if ((ampDS & 32)==32) {	//phon�me avec bruit (fshvzjr), segment suivant
			ampDS-=32;
			nSegDS++;
		} else
			nSegDS=nSegIniDS;
		ptSegDS=tVoix[0]->getPtSeg(nSegDS);
		perioDS=(unsigned char)*ptSegDS++;
	}
	if (marqFS) {
		ampFS=*ptAmpFS & 127;
		if ((ampFS & 32)==32) {	//phon�me avec bruit (fshvzjr), segment suivant
			ampFS-=32;
			nSegFS++;
		} else
			nSegFS=nSegIniFS;
		if ((ampFS & 64)==64) {	//derni�re p�riode du sous-phon�me
			ampFS-=64;
			marqFS=false;	//pour le prochain tour
		}
		ptSegFS=tVoix[0]->getPtSeg(nSegFS);
		perioFS=(unsigned char)*ptSegFS++;
	}
	if (marqDS) ptAmpDS++;
	if (marqFS) ptAmpFS++;
	perioBase=((float)perio.DSDecroit*amp.DSDecroit+(float)perio.FSDecroit*amp.FSDecroit
		+(float)perio.DSCroit*amp.DSCroit+(float)perio.FSCroit*amp.FSCroit)
		/(amp.DSDecroit+amp.FSDecroit+amp.DSCroit+amp.FSCroit);
	perioResult=(unsigned char)(perioBase/mulHauteur+.5);
	if (mCroit)
		xEcrEchelleDeLec-=perioResult*mulHauteur;	//p�riode lue mais pas forc�ment �crite
	return true;
}

//Pr�pare les variables du sous-diphone d�croit suivant
bool Parle::nouveauSousDiph(long& iLecPhonX, char& phonG, char& phonD, char& catG, char& catD, char& nSousDiph,
		unsigned char& nSegIniDS, unsigned char& nSegDS, unsigned char*& ptAmpDS,
		unsigned char& ampDS) {
	short nAmp;

	if (nSousDiph==0) {
		nSousDiph++;
		ptAmpDS=tab->getPtAmp(tab->traAmp(catG, catD));
		ampDS=*ptAmpDS;
		nSegIniDS=tab->traTim(phonG, phonD);
	} else if (nSousDiph==1) {
		nSousDiph++;
		ptAmpDS=tab->getPtAmp(tab->debAmp(catG, catD));
		ampDS=*ptAmpDS;
		nSegIniDS=tab->debTim(phonD);
	} else {
		nSousDiph=0;
		phonG=phonD; catG=catD;
		phonD=phonSuiv(iLecPhonX, phonG);
		if (phonD==-1) return false;
		catD=tab->categ(phonD);
		nAmp=tab->finAmp(catG, catD);
		if (nAmp>100) {
			nAmp-=100;
			nSousDiph++;
		}
		ptAmpDS=tab->getPtAmp(nAmp);
		ampDS=*ptAmpDS;
		if (phonG==YY && phonD>YY && phonD<VR)
			nSegIniDS=tab->traTim(II,EU);
		else
			nSegIniDS=tab->finTim(phonG);
	}
	nSegDS=nSegIniDS;
	return true;
}

//Retourne le n� du phon�me suivant du texte phon�tique
char Parle::phonSuiv(long& iLecPhonX, char phonG) {
	
	if (iLecPhonX<iLecPhon) {	//phon�me d�j� trouv� pour Croit ou D�croit
		iLecPhonX=iLecPhon;
		return phonMem;	//gard� en m�moire
	}	//phon�me suivant
	while (true) {
		phonMem=texPhon[iLecPhon];
		if (phonMem==0) {
			phonMem=-1;
			break;	//fin du texte phon�tique
		}
		phonMem-=65;
		iLecPhon++;
		if (phonMem<YY && phonMem==phonG)
			phonMem=PL;
		if (phonMem==PL)
			allonge+=250;	//+25 ms � d�bit normal (~25 %)
		else if (phonMem+65==INDEX) {
			if (synGlobal.getSortieSon()) {
				synGlobal.setTNEch(synGlobal.getNbIndexEcr(), synGlobal.getCtEch());	//pour index sous linux
				synGlobal.setNbIndexEcr(synGlobal.getNbIndexEcr()-1);	//d�cr�mente le nb d'index restant � lire (� �crire dans tampon)
			}
		}
		else break;	//rien de particulier donc phon�me
	}
	iLecPhonX=iLecPhon;
	if (phonMem==-1 && phonG!=VR) phonMem=VR;
	return phonMem;
}

//Calcule un �chantillon de la p�riode d�croissante.
//x abscisse de l'�chantillon dans la p�riode
short Parle::calculeEchPerioDecroit(short x, char* ptSegDecroit, unsigned char perioDecroit,
		unsigned char ampDecroit) {

	if (ampDecroit==0) return 0;
	short xCadre=x+perioDecroit/4;  //cadrage de la fen�tre sur la partie de forte amplitude
	if (xCadre>=perioDecroit) xCadre-=perioDecroit;
	if (perioResult<=perioDecroit) {	// p�rio r�sultante <= p�rio decroit
		return (short)(fenDecroit((float)x/perioResult)*ptSegDecroit[xCadre]*ampDecroit);
	}
	else {	// p�rio r�sultante > p�rio decroit
		if (x<perioDecroit)
			return (short)(fenDecroit((float)x/perioDecroit)*ptSegDecroit[xCadre]*ampDecroit);
		else
			return 0;	//reste nul derri�re la fen�tre d�croit
	}
}

//Calcule un �chantillon de la p�riode croissante.
//x abscisse de l'�chantillon dans la p�riode
short Parle::calculeEchPerioCroit(short x, char* ptSegCroit, unsigned char perioCroit,
		unsigned char ampCroit) {

	if (ampCroit==0) return 0;
	short xCadre=x+perioCroit-perioResult+perioCroit/4;  //cadrage de la fen�tre sur la partie de forte amplitude
	if (xCadre>=perioCroit) xCadre-=perioCroit;
	if (perioResult<=perioCroit) {	// p�rio r�sultante <= p�rio croit
		return (short)(fenCroit((float)x/perioResult)*ptSegCroit[xCadre]*ampCroit);
	}
	else {	//p�rio r�sultante > p�rio croit
		if (x>=perioResult-perioCroit)
			return (short)(fenCroit((float)(x+perioCroit-perioResult)/perioCroit)*ptSegCroit[xCadre]*ampCroit);
		else
			return 0;	//nul avant la fen�tre croit
	}
}

//////////////////////////////
// Fonctions globales
//////////////////////////////

//Initialise (init) ou termine (!init) la production d'un fichier wave
void initWave(bool init) {
  if (!synGlobal.getSortieWave()) return;
  if (init) {	//pas d'autre test car les �ch suivent
    longWave=0;
    short entier;
    int entierLong;
    // Ouvre un fichier wave sinon envoie vers stdout
    if (synGlobal.getNomFichierWave() != NULL)
      ficWave=fopen((char*)synGlobal.getNomFichierWave(), "wb");
    else {
      ficWave=stdout;
    }
    fwrite("RIFF", sizeof(char), 4, ficWave);
    if (synGlobal.getNomFichierWave() != NULL)
      entierLong=longWave+36;	//taille totale du fichier restant
    else
      entierLong=4 + (8+16) + (8+MS_UNSPEC);
    fwrite(&entierLong, 4, 1, ficWave);
    fwrite("WAVEfmt ", sizeof(char), 8, ficWave);
    entierLong=16;	//taille des param�tres jusqu'� "data"
    fwrite(&entierLong, 4, 1, ficWave);
    entier=1;	//identifie si PCM, ULAW etc
    fwrite(&entier, sizeof(short), 1,ficWave);
    entier=1;	// Nombre de canaux
    fwrite(&entier, sizeof(short), 1,ficWave);
    entierLong=FREQ_ECH;	//fr�q �ch
    fwrite(&entierLong, 4,1,ficWave);
    entierLong=FREQ_ECH*TYPE_ECH/8;	//fr�q octets
    fwrite(&entierLong, 4,1,ficWave);
    entier=TYPE_ECH/8;	//nb octets par �ch
    fwrite(&entier,sizeof(short), 1,ficWave);
    entier=TYPE_ECH;	//8 ou 16 bits
    fwrite(&entier, sizeof(short), 1,ficWave);
    fwrite("data", sizeof(char), 4, ficWave);
    if (synGlobal.getNomFichierWave() != NULL)
      entierLong=longWave;
    else
      entierLong=MS_UNSPEC;
    fwrite(&entierLong, 4, 1, ficWave);
    ficWaveFerme=false;
  } else if (!ficWaveFerme) {	//termine
    if (synGlobal.getNomFichierWave() != NULL) {
      longWave *=TYPE_ECH/8;			// nb octets
      fseek(ficWave, 40, 0);
      fwrite(&longWave, 4, 1, ficWave);
      longWave+=36;
      fseek(ficWave, 4, 0);
      fwrite(&longWave, 4, 1, ficWave);
      fclose(ficWave);
    } else
      fflush(ficWave);
    ficWaveFerme=true;
  }
}

//D�truit synSon apr�s avoir parl�
void sonDestruction() {
	if (synSon) {
#ifdef WIN32
		entreSectionCritiqueGlobal3();	// E E E E E E E E E E E E E
		delete synSon;	//d�truit l'objet son (donc le buffer circulaire et l'objet carte-son snd_dev)
		quitteSectionCritiqueGlobal3();	// Q Q Q Q Q Q Q Q Q Q Q Q Q
#else
		delete synSon;	//d�truit l'objet son (donc le buffer circulaire et l'objet carte-son snd_dev)
#endif
		synSon=NULL;	//pour redondance de sonDestruction
	}
}

