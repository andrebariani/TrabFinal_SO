/*
 * RSFS - Really Simple File System
 *
 * Copyright © 2010 Gustavo Maciel Dias Vieira
 * Copyright © 2010 Rodrigo Rocco Barbieri
 *
 * This file is part of RSFS.
 *
 * RSFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define CLUSTERSIZE 4096

unsigned short fat[65536];

typedef struct {
       char used;
       char name[25];
       unsigned short first_block;
       int size;
} dir_entry;

dir_entry dir[128];

/* Constantes de Agrupamentos */

#define AGRUP_LIVRE 1
#define AGRUP_ULTIMO 2
#define AGRUP_FAT 3
#define AGRUP_DIR 4
#define SIZE_FAT 65536
#define SIZE_DIR 128

/* Funções auxiliaes */
void bytencpy(char *Destino, char *Origem, int Tamanho);

int fs_init() {
  //Carregando FAT
  for(int cluster = 0; cluster < 32; cluster++)
  {
      for(int sector = 0; sector < 8; sector++)
      {
        char buffer[SECTORSIZE];
        bl_read((cluster*8 + sector), buffer);
        bytencpy((char *) fat, buffer, SECTORSIZE);
      }
  }

  //Verficando integridade
  for(int i = 0; i < 32; i++)
  {
    if(fat[i] != AGRUP_FAT)
    {
      return 0;
    }
  }
  if(fat[32] != AGRUP_DIR)
  {
    return 0;
  }

  //Carregando Diretório
      for(int sector = 0; sector < 8; sector++)
      {
        char buffer[SECTORSIZE];
        bl_read(sector + 256, buffer);
        bytencpy((char *) dir, buffer, SECTORSIZE);
      }

  return 1;
}

int fs_format() {

  //Inicializando estruturas em RAM
  //FAT
  for(int i = 0; i < 32; i++)
  {
    fat[i] = AGRUP_FAT;
  }
  fat[32] = AGRUP_DIR;
  for(int i = 33; i < SIZE_FAT; i++)
  {
    fat[i] = AGRUP_LIVRE;
  }

  //Diretório
  for(int i = 0; i < 128; i++)
  {
    dir[i].used = 'F';
    dir[i].name[0] = '\0';
  }

  //Escrevendo no arquivo
  //FAT
  for(int cluster = 0; cluster < 32; cluster++)
  {
    for(int sector = 0; sector < 8; sector++)
    {
      char buffer[SECTORSIZE];
      bytencpy(buffer, (char *) fat, SECTORSIZE);
      bl_write((cluster*8 + sector), buffer);
    }
  }

  //Diretório
  for(int sector = 0; sector < 8; sector++)
  {
    char buffer[SECTORSIZE];
    bytencpy(buffer, (char *) dir, SECTORSIZE);
    bl_write(sector + 256, buffer);

  }

  return 0;
}

int fs_free() {
  /*------------DUVIDA------------*/
  /*Temos que carregar a fat aqui?*/
  int agrupLivres = 0;

  //Contando os empaços livres da FAT
  for (int i = 0; i < SIZE_FAT; i++)
  {
    if(fat[i] == AGRUP_LIVRE)
    {
      agrupLivres++;
    }
  }

  //Multiplicando para tranformar os clusters em bytes
  return agrupLivres * CLUSTERSIZE;
}

int fs_list(char *buffer, int size) {
  buffer[0] = '\0';
  char aux[31];

  for (int i = 0; i < SIZE_DIR; i++)
  {
    if(dir[i].used != 'F')
    {
      sprintf(aux, "%s\t\t%d\n", dir[i].name, dir[i].size);
      strcpy( &( buffer[strlen(buffer)] ), aux);
    }
  }

  return 1;
}

int fs_create(char* file_name) {
  printf("Função não implementada: fs_create\n");
  return 0;
}

int fs_remove(char *file_name) {
  printf("Função não implementada: fs_remove\n");
  return 0;
}

int fs_open(char *file_name, int mode) {
  printf("Função não implementada: fs_open\n");
  return -1;
}

int fs_close(int file)  {
  printf("Função não implementada: fs_close\n");
  return 0;
}

int fs_write(char *buffer, int size, int file) {
  printf("Função não implementada: fs_write\n");
  return -1;
}

int fs_read(char *buffer, int size, int file) {
  printf("Função não implementada: fs_read\n");
  return -1;
}

void bytencpy(char * Destino, char * Origem, int Tamanho)
{
  strncpy(Destino, Origem, Tamanho);
}
