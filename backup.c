#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glob.h>

# define ERROR 1
# define FAILURE -1

int main(int argc, char **argv){
    
    //OBTENER LA FECHA Y HORA
    time_t rawtime = time(NULL);
    if(rawtime == -1){perror("Error obteniendo el tiempo");exit(EXIT_FAILURE);}
    struct tm *local_time = localtime(&rawtime);
    if(local_time == NULL){perror("Error al obtener la hora actual");exit(EXIT_FAILURE);}
    //TRANSFORMAR LA FECHA Y HORA AL FORMATO YYYYMMDDTHHMMSS
    char *datetime = calloc(1,25);
    if(datetime == NULL){perror("Error en calloc 1");exit(EXIT_FAILURE);}
    strftime (datetime, 16, "%Y%m%d-%H%M%S", local_time);

    char *filename = argv[1];
    size_t file_len = strlen(filename) + strlen(datetime) + 2;
    //CREAR EL ARCHIVO DE BACKUP NUEVO
    char *backup_filename = calloc(1, file_len);
    if(backup_filename == NULL){perror("Error en calloc 2");exit(EXIT_FAILURE);}
    sprintf(backup_filename, "%s_%s", filename, datetime);
    free(datetime);

    //OBTENER LA INFORMACION DEL ARCHIVO
    struct stat *buf_file = calloc(1, sizeof(struct stat));
    if(buf_file == NULL){perror("Error en calloc 3");exit(EXIT_FAILURE);}
    int error =  stat(filename, buf_file);
    if(error == -1){perror("Error obteniendo la informacion del archivo");exit(EXIT_FAILURE);}

    //BUSCAR SI EXISTE UN ARCHIVO DE COPIA [0] O UN ENLACE DURO 
    char *pattern = calloc(1, strlen(filename) + 2);
    sprintf(pattern, "%s_*", filename);
    glob_t globlist;
    error = glob(pattern, GLOB_PERIOD, NULL, &globlist);
    free(pattern);

    char *command = calloc(1, file_len * 2);
    if(command == NULL){perror("Error en calloc 4");exit(EXIT_FAILURE);}

    if (error != 0){
        //NO EXISTE UNA COPIA DE SEGURIDAD
        sprintf(command, "cp %s %s", filename, backup_filename);    
        system(command);
        error = chmod(backup_filename, 0444);
        if(error == -1){perror("Error obteniendo la informacion del archivo");exit(EXIT_FAILURE);}

    }else{
        //EXISTE UNA COPIA PREVIA

        //OBTENER LA INFORMACION DE LA COPIA EXISTENTE
        char *old_copy = globlist.gl_pathv[0];
        struct stat *buf_copy = malloc(sizeof(struct stat));
        if(buf_copy == NULL){perror("Error en malloc");exit(EXIT_FAILURE);}
        int error =  stat(old_copy, buf_copy);
        if(error == -1){perror("Error obteniendo la informacion del archivo");exit(EXIT_FAILURE);}

        if (globlist.gl_pathc > 1){
            //EXISTE UN ENLACE DURO
            char *old_hard_link = globlist.gl_pathv[1];
            if(buf_file->st_mtim.tv_sec > buf_copy->st_mtim.tv_sec){
                //HUBO CAMBIOS
                
                remove(old_copy);
                remove(old_hard_link);

                free(command);
                char *command = calloc(1, file_len * 2);
                if(command == NULL){perror("Error en calloc 1");exit(EXIT_FAILURE);}

                sprintf(command, "cp %s %s", filename, backup_filename);
                system(command);    //CREAR UNA NUEVA COPIA
                error = chmod(backup_filename, 0444);
                if(error == -1){perror("Error obteniendo la informacion del archivo");exit(EXIT_FAILURE);}

            }else{
                //NO HUBO CAMBIOS
                
                free(command);
                char *command = calloc(1, file_len * 2);
                if(command == NULL){perror("Error en calloc 1");exit(EXIT_FAILURE);}

                sprintf(command, "mv %s %s", old_hard_link, backup_filename);
                system(command);    //CREAR UNA NUEVA COPIA
                error = chmod(backup_filename, 0444);
                if(error == -1){perror("Error obteniendo la informacion del archivo");exit(EXIT_FAILURE);}

            }
        }else{
            //SOLO EXISTE LA COPIA

            //COMPROBAR SI HUBO CAMBIOS DESDE LA ULTIMA COPIA DE SEGURIDAD
            if(buf_file->st_mtim.tv_sec > buf_copy->st_mtim.tv_sec){
                //HUBO CAMBIOS
                remove(old_copy);
                free(command);
                char *command = calloc(1, file_len * 2);
                if(command == NULL){perror("Error en calloc 1");exit(EXIT_FAILURE);}

                sprintf(command, "cp %s %s", filename, backup_filename);
                system(command);    //CREAR UNA NUEVA COPIA
                error = chmod(backup_filename, 0444);
                if(error == -1){perror("Error obteniendo la informacion del archivo");exit(EXIT_FAILURE);}

            }else{
                //NO HUBO CAMBIOS
                free(command);
                char *command = calloc(1, file_len * 2);
                if(command == NULL){perror("Error en calloc 1");exit(EXIT_FAILURE);}

                sprintf(command, "ln %s %s", old_copy, backup_filename);
                system(command);   //CREAR EL HARD LINK 
                error = chmod(backup_filename, 0444);
                if(error == -1){perror("Error obteniendo la informacion del archivo");exit(EXIT_FAILURE);}
            }
        }
        free(buf_copy);
        free(buf_file);
    }
    free(backup_filename);
    free(command);

    return 0;
}

