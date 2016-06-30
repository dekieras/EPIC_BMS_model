/*
 *  Message.h
 *  BrungartV2_device
 *
 *  Created by David Kieras on 7/19/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef MESSAGE_H
#define MESSAGE_H


#include "CRM_utterance_stats.h"
#include <vector>
#include "EPICLib/Symbol.h"

class Device_base;


typedef std::vector<Symbol> Words_t;
typedef std::vector<Words_t> Messages_t;

// a package of information about a message, pulling color and digit names from the supplied response object
struct Message {
	Message();
    Message(Device_base * device_ptr_, const Symbol& stream_name_, const Symbol& gender_, const Symbol& speaker_id_,
        int talker_idx_, int callsign_idx_, int color_idx_, int digit_idx_,
		const CRM_utterance_stats& utterance_stats,
		double baseline_loudness,
//		double mean_loudness_, double sd_loudness_, 
//		double mean_pitch_, double sd_pitch_,
		const Symbol& callsign_, const Symbol& color_, const Symbol& digit_);

	void initialize();
	
	// present the  word
	void present_word(int trial, int word_counter);
	// return the duration for the specified word
	static long get_duration(int word_counter);
	
	Symbol stream_name;
	Symbol speaker_gender;
	Symbol speaker_id;
    int talker_idx;
    int callsign_idx;
    int color_idx;
    int digit_idx;
	Symbol callsign;
	Symbol color;
	Symbol digit;
	Words_t message;					// the actual word sequence
	// for this message
	std::vector<double> loudnesses;
	std::vector<double> pitches;
//	std::vector<long> durations;
	
	
private:
	Device_base * device_ptr;	// must be initialized at construction
	// shared by all messages
	static Words_t skeleton;
	static std::vector<long> durations;  // assuming all segment durations are equal
	
};


#endif
