/*
 *  CRM_utterance_stats.cpp
 *  BrungartV3_device
 *
 *  Created by David Kieras on 5/14/12.
 *  Copyright 2012 University of Michigan. All rights reserved.
 *
 */

#include "CRM_utterance_stats.h"
#include "EPICLib/Numeric_utilities.h"
#include <iostream>


std::istream& operator>>(std::istream& infile, CRM_utterance_stats& stats)
{
	infile >> stats.duration;
	for(int i = 0; i < n_utterance_segments; i++) {
        double raw_pitch;
		infile >> stats.loudnesses[i] >> raw_pitch;
        // convert pitches to semitones
        stats.pitches[i] = pitch_to_semitones(raw_pitch);
		} 
	return infile;
}

std::ostream& operator<<(std::ostream& os, const CRM_utterance_stats& stats)
{
	os << stats.duration << ' ';
	for(int i = 0; i < n_utterance_segments; i++) {
		os << stats.loudnesses[i]  << ' ' << stats.pitches[i] << ' ';
		} 
	return os;
}
