#ifndef PERC_H
#define PERC_H

#include <string.h>
#include <stdlib.h>

static char char_map[0x5D+1];

void perc_init(){
    memset(char_map, 0, sizeof(char_map));

    char_map[0x21] = '!';
    char_map[0x23] = '#';
    char_map[0x24] = '$';
    char_map[0x25] = '%';
    char_map[0x26] = '&';
    char_map[0x27] = '\\';
    char_map[0x28] = '(';
    char_map[0x29] = ')';
    char_map[0x2A] = '*';
    char_map[0x2B] = '+';
    char_map[0x2C] = ',';
    char_map[0x2F] = '/';
    char_map[0x3A] = ':';
    char_map[0x3B] = ';';
    char_map[0x3C] = '<';
    char_map[0x3D] = '=';
    char_map[0x3E] = '>';
    char_map[0x3F] = '?';
    char_map[0x40] = '@';
    char_map[0x5B] = '[';
    char_map[0x5D] = ']';
}

char* perc_convert(char *buf, uint size){
    char *new_buf = NULL;
    uint new_buf_size = 0;
    char tmp_buf[size];
    uint count = 0;

    memcpy(tmp_buf, buf, size);

    for(uint i = 0; i < size; i++){
        if(tmp_buf[i] == '%'){
            if(i+2 < size){
                char hex[5] = {'0', 'x', tmp_buf[i+1], tmp_buf[i+2], '\0'};
                uint val = strtoul(hex, NULL, 16);
                if(val <= 0x5D){
                    tmp_buf[i] = char_map[val];
                    tmp_buf[i+1] = 0;
                    tmp_buf[i+2] = 0;
                    count++;
                }
            }
        }
    }

    new_buf_size = (size - (count*2)) + 2;

    new_buf = (char*)malloc(new_buf_size);
    memset(new_buf, 0, new_buf_size);

    for(int i = 0, j = 0;i < size; i++){
        if(tmp_buf[i] != 0){
            new_buf[j] = tmp_buf[i];
            j++;
        }
    }

    return new_buf;
}

#endif //PERC_H