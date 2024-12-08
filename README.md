
## Nume
**SmartKnob: sistem haptic de control inteligent**
GITLAB: https://gitlab.upt.ro/vlad.plavat/smartknob
GITHUB: https://github.com/vlad-plavat/SmartKnob
Repo-ul de GitLab conține un link către codul sursă aflat pe GitHub.

## Descriere
Sistemul dezvoltat reprezintă un dispozitiv pe intrare pentru PC. Acesta este dotat cu feedback haptic. Alimentarea și comunicarea prin USB permit o simplă conectare și utilizare. Este compatibil cu majoritatea sistemelor de operare folosind clasa USB HID. Folosind acest dispozitiv se pot crea interfețe complexe pentru utilizatori.


## Compilarea codului pentru microcontroler
1. Se va instala mediul Pico SDK pentru Visual Studio Code.
2. Se va clona repo-ul de pe GitHub:
   git clone https://github.com/vlad-plavat/SmartKnob <br>
3. Se va deschide folderul firmware în Visual Studio Code.
4. Se va executa comanda de build pentru a rula CMakeLists.txt
5. Fișierul SmartKnob.uf2 din folderul build creat automat se va copia pe Raspberry Pi Pico



## Rularea aplicației în Unity
   1. Se va deschide proiectul Unity folosind Unity Hub.
   Proiectul se găsește în folderul test_app.
   2. Se instalează HidLibrary în Unity.
   3. Din aplicația Unity se va da click pe butonul de Run pentru a rula aplicația.
   4. Eventual, se poate compila aplicația într-o formă executabilă folosind Ctrl+B sau meniul Unity.


## Autor și Coordonator
student: Vlad Plăvăț
coordonator: dr.ing Valentin Stângaciu


