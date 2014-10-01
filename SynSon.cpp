/*
 * Synth� 3 - A speech synthetizer software for french
 *
 * Copyright (C) 1985-2014 by Michel MOREL <michel.morel@unicaen.fr>.
 *
 * Synth� 3 comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: https://github.com/acceslibreinfo/synthe3
 *
 * This software is maintained by Sh�rab <Sebastien.Hinderer@ens-lyon.org>.
 */

////////////////////////////////////
// Sortie vers la carte-son
// (commune � Alsa et Direct-Sound)
////////////////////////////////////

#include "SynSon.h"
#include "SynGlobal.h"
#include "SynMain.h"
#ifndef WIN32
	#include <unistd.h>
#endif

classSon* synSon;

//Constructeur : initialise les param�tres des �chantillons, les variables globables et les objets de gestion du son.
//Prend en param�tre la fr�q �ch, le nb de voies (MONO ou STEREO) et le type des �chantillons (16 ou 8 bits).
classSon::classSon(int frequence, short nbVoies, short typeEchVoie) {
	snd_ok = false;	//direct sound pas encore essay� d'ouvrir
	iEchEcr = 0;	//position �criture au d�but du buffer (en nb de paquets)
	iEchPaquet = 0;	//position dans le paquet
	iEchPosLec=0;	//position de lecture
	iEchPosLecSauv=0;	//copie pour restauration
	iEchPosLecAv=0;	//indice lecture pr�c�dent (avant positionLecture)
	marqJoueInit = false;	//n'a pas commenc� � jouer
	marqJoue = false;	//ne joue pas
	pauseEnCours = false;	//pas de pause en cours
	// Param�tres du son
	fEchCarte = frequence;	// 44100, 22050 ou 11025
//	fEchCarte=22050;	//permet de forcer la fr�quence de la carte et d'adapter la sortie des �ch
	nbVoiesSon = nbVoies;	// MONO ou STEREO
	nbBitsParEchVoie = typeEchVoie;	// 8 ou 16 bits
	nbOctetsParEch = nbBitsParEchVoie*nbVoiesSon/8;
	// Param�tres du tampon
	short nbEchUnite=(short)(UNITE_TEMPS*fEchCarte+.5);	//2 (ne sert que pour les arrondis)
	nbEchPaquet = (long)(TEMPS_PAQUET*fEchCarte+.5)/nbEchUnite*nbEchUnite;
#ifndef WIN32
	nbEchPaquet=2048;
#endif
	nbEchSecuEcr = (long)(TEMPS_SECU_ECR*fEchCarte+.5)/nbEchUnite*nbEchUnite;
	nbEchSecuLec = (long)(TEMPS_SECU_LEC*fEchCarte+.5)/nbEchUnite*nbEchUnite;
	nbPaquetsLpBuffer=(short)((long)(TEMPS_TAMPON*fEchCarte+.5)/nbEchPaquet);
#ifndef WIN32
	nbPaquetsLpBuffer=8;
#endif
	nbEchLpBuffer = nbPaquetsLpBuffer*nbEchPaquet;
	nbEchLpBuffer2=nbEchLpBuffer/2;
	nbOctetsPaquet = nbEchPaquet*nbOctetsParEch;
	// Cr�e le paquet
	switch(nbBitsParEchVoie) {
	case 8 : paquet8 = new char[nbEchPaquet*nbVoiesSon]; break;
	case 16 : paquet16 = new short[nbEchPaquet*nbVoiesSon]; break;
	}
	// Cr�e le tableau d'index
	tIndex = new short[nbPaquetsLpBuffer];
	tIndex[0]=synGlobal.getNbIndexEcr();
	//Divers init
	echTot=0;
	nbTot=0;
	cumul=0;
	open_snd();
	set_snd_params(nbVoies,typeEchVoie,frequence);
}

//Destructeur son
classSon::~classSon() {
	if (!snd_ok) return;
	snd_ok=false;
	switch(nbBitsParEchVoie) {
		case 8 : delete[] paquet8; break;
		case 16 : delete[] paquet16; break;
	}
	delete[] tIndex;
	close_snd();
}

//Son ouvert avec succ�s (idem pour ferm�)
bool classSon::ouvertOK() {
	return snd_ok;
}

//Indique que Synth� doit finir de lire le buffer jusqu'� la limite d'�criture
void classSon::finirSon(bool marqFin) {
	if (marqFin) {
		etatSon=1;	//lit jusqu'� la limite limLit
		limLit=iEchEcr;	//arrondi � la taille du paquet
		limLitPaquet=iEchPaquet;	//position dans le paquet
	} else
		etatSon=0;	//pr�t pour lecture (ce n'est pas la fin)
}

//Transf�re les �chantillons dans le buffer secondaire.
//Prend en param�tre l'�chantillon � envoyer � la carte son. 2 valeurs en st�r�o
//En cas de stop, return false pour stopper tout
//bool classSon::transfert(LPVOID ptEchG, LPVOID ptEchD) {	//localisation
bool classSon::transfert(LPVOID ptEch) {
	synGlobal.incrCtEch();	//incr�mente le compteur d'�chantillons pour g�rer le tableau d'index (position de lecture sous Linux)
	if (iEchPaquet==0) {
		indexAEcrire=synGlobal.getNbIndexEcr();	//met � jour l'index � �crire � chaque nouveau paquet
		//Sleep(30);	//ralentit l'�criture pour test gestion lecture
	}
	if (synGlobal.getDemandeStop()) {
		pauseSiJoue();
		return false;	//le stop agit imm�diatement (� chaque �chantillon)
	}
	if (etatSon>0 && iEchPaquet==0) {	//cas de la fin normale du message, nouveau paquet
		positionLecture();	//demande position � la carte son (->iEchPosLec) (une fois par paquet sinon ralentit)
		/*if (!marqJoueInit)
			iEchPosLec=0;	//redondant au cas o� positionLecture retournerait une valeur fausse
		else if (!marqJoue)
			iEchPosLec=iEchPosLecAv;	//idem*/
		//Si l'UC a �t� sollicit�e, la lecture peut avoir l�g�rement d�pass� la limite, d'o� les pr�cautions qui suivent
		if (etatSon==1) {	//1er �tat de fin : avant bouclage
			if (iEchPosLec<limLit || iEchPosLec<iEchPosLecAv-nbEchLpBuffer2) {
				//Si la lecture est avant la limite ou a boucl�
				etatSon=2;	//indique que la lecture ne va plus boucler
				limLit+=limLitPaquet;	//position exacte de la fin, mais il fallait attendre que la lecture ait avanc� (et reparte pour un tour)
					//car le paquet n'est pas encore �crit et la limite r�elle est en avance (UC rapide) (sinon la derni�re seconde est coup�e)
			}	//Sinon : la lecture est derri�re la limite, on ne fait rien, on attend le bouclage
		}
		else if (etatSon==2) {	//2e �tat de fin: apr�s bouclage
			if (iEchPosLec>=limLit || iEchPosLec<iEchPosLecAv-nbEchLpBuffer2) {
				//Si la lecture a rattrap� ou d�pass� la limite ou boucl�, c'est fini
				pauseSiJoue();	//stop normal car lecture termin�e (sous directSound)
				return false;
			}	//Sinon : la lecture n'a pas atteint la limite, on attend
		}
		iEchPosLecAv=iEchPosLec;   	//rep�re la derni�re position de lecture pour d�tecter bouclage plus tard
	}
	// Ajoute un �ch dans le paquet
	switch (nbBitsParEchVoie) {
	case 8 : 
		paquet8[iEchPaquet*nbVoiesSon] = *((char*) ptEch);
		if (nbVoiesSon==STEREO)
			paquet8[iEchPaquet*nbVoiesSon+1] = *((char*) ptEch);
		break;
	case 16 :
		paquet16[iEchPaquet*nbVoiesSon] = *((short*)ptEch);
		if (nbVoiesSon==STEREO)
			paquet16[iEchPaquet*nbVoiesSon+1] = *((short*) ptEch);
		break;
	}
	iEchPaquet++;
	// Si un paquet est rempli, on le transf�re dans lpBuffer
	if (iEchPaquet == nbEchPaquet) {
		// Contr�le la lecture et l'�criture
		attendSiEcrOuLecRattrape();	//les pointeurs de lecture et d'�criture ne doivent pas se doubler
		// Ecrit l'index dans un tableau circulaire (du nb de paquets)
		tIndex[iEchEcr/nbEchPaquet] = indexAEcrire;
		// Transf�re le paquet plein dans lpBuffer
		entreSectionCritiqueGlobal();	// E E E E E E E E E E E E E
		switch (nbBitsParEchVoie) {
		case 8 :
#ifdef WIN32
			transferePaquet(lpBuffer, nbOctetsParEch*iEchEcr, paquet8, nbOctetsPaquet);
#else
			transferePaquet(paquet8, nbOctetsPaquet);
#endif
			break;
		case 16 :
#ifdef WIN32
			transferePaquet(lpBuffer, nbOctetsParEch*iEchEcr, paquet16, nbOctetsPaquet);
#else
			transferePaquet(paquet16, nbOctetsPaquet);
#endif
			break;
		}
		quitteSectionCritiqueGlobal();	// Q Q Q Q Q Q Q Q Q Q Q Q
		// Met � jour les positions
		iEchEcr += nbEchPaquet;
		if (iEchEcr >= nbEchLpBuffer)
			iEchEcr = 0;	//l'�criture boucle
		iEchPaquet = 0;	//d�marre un nouveau paquet
	}
	return true;
}

//Attend que la position de lecture ait pris au moins un paquet d'avance sur la position d'�criture.
//Arr�te l'�criture si elle rattrape la lecture (UC rapide)
//Arr�te la lecture si elle rattrape l'�criture (UC ralentie) (pause sinon disque ray�)
//Utilis� dans transfert
void classSon::attendSiEcrOuLecRattrape() {
	short nbPaquetsSecuEcr=nbEchSecuEcr/nbEchPaquet;	//nb paquets d'�cart min quand ecr rattrape lec
	short nbPaquetsSecuLec=nbEchSecuLec/nbEchPaquet;	//nb paquets d'�cart min quand lec rattrape ecr
	short iPaquetEcr;
	short iPaquetLecAvant;	//rep�re de s�curit� avant le paquet en cours de lecture
	short iPaquetLecApres;	//rep�re de s�curit� apr�s le paquet en cours de lecture
	short iPaquetLec;

#ifdef WIN32
	initWindow();	//si le handle de la fen�tre courante � chang�, l'objet DirectSound est remis � jour
#endif
	//Si la lecture est en cours, attend qu'il y ait la place d'�crire un paquet plein sans d�border pt lecture
	while (true) {
		//R�cup�re la position de lecture
		positionLecture();	//fournit iEchPosLec (� chaque paquet termin�)
		/*if (!marqJoueInit)
			iEchPosLec=0;	//redondant pour Jaws au cas o� positionLecture retournerait une valeur fausse
		else if (!marqJoue)
			iEchPosLec=iEchPosLecAv;	//essayer : ne donne rien*/
		iEchPosLecAv=iEchPosLec;   	//rep�re la derni�re position de lecture pour d�tecter bouclage plus tard
		iPaquetLec=(short)(iEchPosLec/nbEchPaquet);
		iPaquetEcr=(short)(iEchEcr/nbEchPaquet);
		iPaquetLecAvant=iPaquetLec-nbPaquetsSecuEcr;	//rep�re � ne pas d�passer en �criture
		if (iPaquetLecAvant<0)
			iPaquetLecAvant+=nbPaquetsLpBuffer;	//bouclage
		iPaquetLecApres=iPaquetLec+nbPaquetsSecuLec;	//rep�re
		if (iPaquetLecApres>=nbPaquetsLpBuffer)
			iPaquetLecApres-=nbPaquetsLpBuffer;	//bouclage
		if (!marqJoue)
			break;	//si on ne joue pas, on peut �crire
		//Si on joue, met � jour l'index (au moins une fois par paquet) pour indiquer � Synth� o� il en est
#ifdef WIN32
		synGlobal.setNbIndexLec(tIndex[iEchPosLec/nbEchPaquet]);
#else
		//Ou le calcule en fonction du temps pour Linux
		struct timeval tv;
		float n=0;
		gettimeofday(&tv, NULL);
		float a=(tv.tv_sec-synGlobal.getktime0s())*1000000;
		float b=tv.tv_usec-synGlobal.getktime0us();
		n=a+b;	//en �s
		n/=1000000.0;	//en s
		n*=22050.0;	//en �chantillons
		short i=0;
		int nbIndexMax=synGlobal.getNbIndexMax();
		for (i=0; i<=nbIndexMax; i++) {
			if (synGlobal.getTNEch(i)>0 && synGlobal.getTNEch(i)<n) {
				break;
			}
		}
		synGlobal.setNbIndexLec(i);
#endif
		//Quand on joue, tant que le paquet � transf�rer est dans le secteur qui pr�c�de le paquet en cours de lecture, on attend pour �crire
		if ((iPaquetLecAvant<iPaquetLec &&	//si les 2 rep�res dans la m�me boucle
			(iPaquetEcr<iPaquetLecAvant ||	//et �criture avant le premier rep�re
			iPaquetEcr>iPaquetLec)) ||	//ou apr�s la lecture -> on peut �crire
			(iPaquetLecAvant>iPaquetLec &&	//si bouclage entre les 2 rep�res
			iPaquetEcr<iPaquetLecAvant &&	//et �criture avant le 1er rep�re
			iPaquetEcr>iPaquetLec))	//et derri�re la lecture -> on peut �crire
			break;	//on peut �crire
		Sleep(0);	//en cas d'attente, on donne la main aux autres threads
	}
	if (!marqJoueInit) {	//pas commenc�
		if (iPaquetEcr>iPaquetLecApres) {
			joueSiPause();	//si l'�cart est suffisant, on d�marre
			marqJoueInit=true;	//c'est commenc�
		}
	//Si le paquet � transf�rer est dans le secteur qui suit le paquet en cours de lecture, on arr�te la lecture le temps qu'il s'�loigne
	} else if ((iPaquetLecApres>iPaquetLec &&	//si les 2 rep�res dans la m�me boucle
		iPaquetEcr<=iPaquetLecApres &&	//et �criture entre les 2 rep�res
		iPaquetEcr>iPaquetLec) ||	//donc pas assez loin de la lecture -> pause lecture
		(iPaquetLecApres<iPaquetLec &&	//si bouclage entre les 2 rep�res
		(iPaquetEcr<=iPaquetLecApres ||	//et �criture devant le 1er rep�re
		iPaquetEcr>iPaquetLec)))	//ou derri�re la lecture (donc pas assez loin) -> pause lecture
		pauseSiJoue();	//si la lecture rattrape l'�criture, on la stoppe un moment.
	else
		joueSiPause();	//sinon, l'�cart est suffisant, on reprend
}

