/*
 * structures.h
 *
 * Created: 24.10.2019 12:11:04
 *  Author: iw851247
 */ 


#pragma once

#include <stdint.h>

typedef enum {
	AVAILABLE,
	BACKORDER,
	SOLD_OUT = 99,
} DistributionStatus;

typedef struct {
	uint8_t manufactureId;
	uint8_t productID;
} ArticleNumber;

typedef union {
	uint16_t combinedNumber;
	ArticleNumber singleNumbers;
} FullArticleNumber;

struct Article {
	FullArticleNumber articleNumber;
	DistributionStatus status;
};

void displayArticles();