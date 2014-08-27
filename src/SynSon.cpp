////////////////////////////////////
// Sortie vers la carte-son
////////////////////////////////////

#include "SynSon.h"
#include "SynGlobal.h"

classSon* synSon;

//Constructeur : initialise les param�tres des �chantillons, les variables globables et les objets de gestion du son.
//Prend en param�tre la fr�q �ch, le nb de voies (MONO ou STEREO) et le type des �chantillons (16 ou 8 bits).
classSon::classSon(int frequence, short typeEchVoie) {
	snd_ok = false;	//direct sound pas encore essay� d'ouvrir
	iEchEcr = 0;	//position �criture au d�but du buffer (en nb de paquets)
	iEchPaquet = 0;	//position dans le paquet
	iEchPosLec=0;	//position de lecture
	iEchPosLecAnc=0;	//copie pour restauration
	iEchPosLecAv=0;	//indice lecture pr�c�dent (avant positionLecture)
	marqJoueInit = false;	//n'a pas commenc� � jouer
	marqJoue = false;	//ne joue pas
	pauseEnCours = false;	//pas de pause en cours
	// Param�tres du son
	fEchCarte = frequence;	// 44100, 22050 ou 11025
//	fEchCarte=22050;	//permet de forcer la fr�quence de la carte et d'adapter la sortie des �ch
	nbVoiesSon = 1;	// 1 MONO ou 2 STEREO
	nbBitsParEchVoie = typeEchVoie;	// 8 ou 16 bits
	nbOctetsParEch = nbBitsParEchVoie*nbVoiesSon/8;
	// Param�tres du tampon
	short nbEchUnite=(short)(UNITE_TEMPS*fEchCarte+.5);	//2
	nbEchPaquet = (long)(TEMPS_PAQUET*fEchCarte+.5)/nbEchUnite*nbEchUnite;
	nbEchSecu = (long)(TEMPS_SECU*fEchCarte+.5)/nbEchUnite*nbEchUnite;
	nbPaquetsLpBuffer=(short)((long)(TEMPS_TAMPON*fEchCarte+.5)/nbEchPaquet);
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
	set_snd_params(nbVoiesSon, typeEchVoie, frequence);
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
		etatFin=1;	//lit jusqu'� la limite limLit
		limLit=iEchEcr;	//arrondi � la taille du paquet
		limLitPaquet=iEchPaquet;	//position dans le paquet
	} else
		etatFin=0;	//en cours de lecture
}

//Transf�re les �chantillons dans le buffer secondaire.
//Prend en param�tre l'�chantillon � envoyer � la carte son. 2 valeurs en st�r�o
//En cas de stop, return false pour stopper tout
//bool classSon::transfert(LPVOID ptEchG, LPVOID ptEchD) {	//localisation
bool classSon::transfert(LPVOID ptEch) {
	synGlobal.incrCtEch();	//incr�mente le compteur d'�chantillons pour g�rer le tableau d'index (position de lecture sous Linux)
	if (iEchPaquet==0)
		indexAEcrire=synGlobal.getNbIndexEcr();	//met � jour l'index � �crire � chaque nouveau paquet
	if (synGlobal.getDemandeStop()) {
		pauseSiJoue();
		return false;	//le stop agit imm�diatement (� chaque �chantillon)
	}
	if (etatFin>0) {	//cas de la fin normale du message
		positionLecture();	//demande position � la carte son (->iEchPosLec) (� chaque fois pour plus de finesse en fin de son)
		//Si l'UC a �t� sollicit�e, la lecture peut avoir l�g�rement d�pass� la limite, d'o� les pr�cautions qui suivent
		if (etatFin==1) {	//1er �tat de fin : avant bouclage
			if (iEchPosLec<limLit || iEchPosLec<iEchPosLecAv-nbEchLpBuffer2) {
				//Si la lecture est avant la limite ou a boucl�
				etatFin=2;	//indique que la lecture ne va plus boucler
				limLit+=limLitPaquet;	//position exacte de la fin, mais il fallait attendre que la lecture ait avanc� (et reparte pour un tour)
					//car le paquet n'est pas encore �crit et la limite r�elle est en avance (UC rapide) (sinon la derni�re seconde est coup�e)
			}
		}	//Sinon : la lecture est derri�re la limite, on ne fait rien, on attend le bouclage
		else if (etatFin==2) {	//2e �tat de fin: apr�s bouclage
			if (iEchPosLec>=limLit || iEchPosLec<iEchPosLecAv-nbEchLpBuffer2) {
				//Si la lecture a rattrap� ou d�pass� la limite, c'est fini
				pauseSiJoue();	//stop normal car lecture termin�e (sous directSound)
				return false;
			}
		}	//Sinon : la lecture n'a pas atteint la limite, on attend
		iEchPosLecAv=iEchPosLec;   	//rep�re la derni�re position de lecture pour rep�rer bouclage plus tard
	}
	// Ajoute un �ch dans le paquet
	switch (nbBitsParEchVoie) {
	case 8 : 
		paquet8[iEchPaquet*nbVoiesSon] = *((char*)ptEch);
		break;
	case 16 :
		paquet16[iEchPaquet*nbVoiesSon] = *((short*)ptEch);
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
		if (iEchEcr == nbEchLpBuffer)
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
	short nbPaquetsEcart=nbEchSecu/nbEchPaquet;	//nb paquets d'�cart min quand lec rattrape ecr (nbPaquetsEcart+1 paquets � pr�parer avant d�but lec)
	short iPaquetEcr;
	short iPaquetEcrAvant;	//paquet �crit nbPaquetsEcart paquets avant
	short iPaquetLec;

#ifdef WIN32
	initWindow();	//si le handle de la fen�tre courante � chang�, l'objet DirectSound est remis � jour
#endif
	//Si la lecture est en cours, attend qu'il y ait la place d'�crire un paquet plein sans d�border pt lecture
	while (true) {
		//R�cup�re la position de lecture
		positionLecture();	//fournit iEchPocLec (� chaque paquet termin�)
		//Met � jour l'index (au moins une fois par paquet) pour indiquer � Synth� o� il en est
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
		//Tant que le paquet � transf�rer recouvre le paquet en cours de lecture, on attend
		iPaquetLec=(short)(iEchPosLec/nbEchPaquet);
		iPaquetEcr=(short)(iEchEcr/nbEchPaquet);
		if (iPaquetLec!=iPaquetEcr || !marqJoue)	//si les num�ros de paquet ne sont pas �gaux (ou si on ne joue pas)
			break;	//on peut �crire
		Sleep(0);	//en cas d'attente, on donne la main aux autres threads
	}
	iPaquetEcrAvant=iPaquetEcr-nbPaquetsEcart;	//rep�re
	if (iPaquetEcrAvant<0)
		iPaquetEcrAvant+=nbPaquetsLpBuffer;	//bouclage
		if ((iPaquetEcrAvant<iPaquetEcr &&	//si les 2 rep�res dans la m�me boucle
		iPaquetLec>=iPaquetEcrAvant &&	//et lecture entre les 2 rep�res
		iPaquetLec<iPaquetEcr) ||	//donc proche de l'�criture -> pause
		(iPaquetEcrAvant>iPaquetEcr &&	//si bouclage entre les 2 rep�res
		(iPaquetLec>=iPaquetEcrAvant ||	//et lecture derri�re le 1er rep�re
		iPaquetLec<iPaquetEcr)))	//ou devant la lecture (donc proche) -> pause
		pauseSiJoue();	//si la lecture rattrape l'�criture, on la stoppe un moment
	else if (iPaquetLec!=iPaquetEcr)
		joueSiPause();	//sinon, si l'�cart est suffisant, on d�marre ou on reprend
}

