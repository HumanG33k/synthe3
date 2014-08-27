#ifndef SYN_CALCUL_H_
#define SYN_CALCUL_H_

////////////////////////////////////////////////////
// Synth� : fonctions math�matiques
////////////////////////////////////////////////////

//Constantes : valeurs de r�f�rence
#define VOLUME_REF 10	//devrait �tre 15, mais on le pousse un peu
#define HAUTEUR_REF 6
#define DEBIT_REF 3

//Constantes math�matiques
const double PI=3.14159;

//Fonctions
extern float fenCroit(float x);
extern float fenDecroit(float x);

#endif
