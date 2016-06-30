/*
 *  CRM_utterance_stats.h
 *  BrungartV3_device
 *
 *  Created by David Kieras on 5/14/12.
 *  Copyright 2012 University of Michigan. All rights reserved.
 *
 */

#ifndef CRM_UTTERANCE_STATS_H
#define CRM_UTTERANCE_STATS_H

#include <iosfwd>


const int n_utterance_segments = 6;

struct CRM_utterance_stats {
	double duration;
	double loudnesses[n_utterance_segments];
	double pitches[n_utterance_segments];
};

std::istream& operator>> (std::istream& infile, CRM_utterance_stats& stats);
std::ostream& operator<<(std::ostream& os, const CRM_utterance_stats& stats);

#endif
