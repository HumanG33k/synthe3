////////////////////////////////////////////////////
// Synth� : fonction parole
////////////////////////////////////////////////////

#include "SynMain.h"
#include "SynParle.h"
#include "SynTrans.h"

//Variables de SynTex.cpp
char tamponAlpha[NM_CAR_TEX];
char tamponPhon[NM_CAR_TEX];

//Synth�se � partir du texte
void synTex(void* lpPara) {
	typeParamThread* lpParamThread=(typeParamThread*)lpPara;

	synGlobal.setThreadOK(true);
	synGlobal.setNbIndexLec(1);	//valeur de d�part (pour cas phon�tique, sans index)
	//Positionne les param�tres
	synGlobal.setPhon(lpParamThread->phon);
	synGlobal.setVolume(lpParamThread->volume);
	synGlobal.setDebit(lpParamThread->debit);
	synGlobal.setHauteur(lpParamThread->hauteur);
	synGlobal.setModeLecture(lpParamThread->modeLecture);
	synGlobal.setModeCompta(lpParamThread->modeCompta);
	synGlobal.setSortieSon(lpParamThread->sortieSon);
	synGlobal.setSortieWave(lpParamThread->sortieWave);
	synGlobal.setNomFichierWave(lpParamThread->nomFicWave);
	Transcription* synTranscription=new Transcription;
	if (!synGlobal.getPhon()) {	//si le texte est alphab�tique
		synTranscription->minMajNFois(lpParamThread->texte, tamponAlpha);	//min->maj et n fois
		synTranscription->graphemePhoneme(tamponAlpha, tamponPhon);	//phon�tise
	} else
		synTranscription->phonemePhoneme(lpParamThread->texte, tamponPhon);	//phon�tise
	delete synTranscription;
	initWave(true);	//initialise un �ventuel fichier wave
	Parle* synParle=new Parle;
	synParle->traiteTextePhonetique(tamponPhon);	//prononce le texte phon�tique
	delete synParle;
	initWave(false);	//termine un �ventuel fichier wave
	sonDestruction();	//d�truit l'objet classSon
	delete[] lpParamThread->texte;
	delete (typeParamThread*)lpPara;
	synGlobal.setNbIndexLec(-1);	//indique la fin de la lecture
}

