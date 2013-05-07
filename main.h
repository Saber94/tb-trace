#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LINE_MAX 		100
#define CODE_GEN_MAX_BLOCKS 1024
#define F_LENGTH 		20
#define NB_SEG 		32
#define CACHE_MAX_BLOCKS 512

#define ADRESS 		0
#define NB_EXEC 		1
#define NB_TRANS 		2
#define LAST_EXEC 	3
#define VALIDE 		4
#define SECOND_CH 	5
#define TRACE_ROWS 	6

#define COLD 			0
#define HOT				1


#define BASIC_MODE	'1'
#define LRU_MODE		'2'
#define LFU_MODE   	'3'
#define MQ_MODE	   '4'




#endif  /* MAIN_H */