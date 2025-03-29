#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int factoriel (int n){
    if (n==0){
        return 1;
    } else {
        return factoriel(n-1)*n;
    }
}

int main(int argc, char** argv){
    if (argc<2){
        printf("il manque 1 argument");
        return 1;
    }
    int arg = atoi(argv[1]);
    int res = factoriel(arg);
    printf("Je suis et foo et je m'execute\n");
    printf("Voici le resultat de fibonnacci de %d : %d\n", arg, res);

    int fd = open("resultat_factoriel.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // Écriture dans le fichier
    char chaine[12]; // Taille suffisante pour stocker les chiffres + '\0'
    sprintf(chaine, "%d", res); // Écrit "123" dans `chaine`

    ssize_t nb_octets = write(fd, chaine, strlen(chaine));
    if (nb_octets == -1) {
        perror("write");
        close(fd);
        return 1;
    }

    // Fermeture du fichier
    if (close(fd) == -1) {
        perror("close");
        return 1;
    }
    


    return 0;
}

